/*
 * shout-out to beej's guide
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include "dispatch.h"
#include "game.h"
#include "handler.h"
#include "message.h"
#include "utils.h"

#define MY_PORT "42069"
#define BACKUP_PORT "42068"
#define BIGGISH_NUMBER 50
#define BACKLOG 20

/*
 * MAIN RETURN VALUES:
 * 0 - normal
 * 1 - user quit early
 */

int play_game(game g, pthread_t *threads) {
    char *inp;
    message m;
    while(1) {
        inp = get_one_stdin_line();
        if (inp == NULL) {
            break;
        }
        m = parse_message(inp);
        free(inp);
        if (m == NULL) {
            continue;
        }
        if (m->command != NULL && strlen(m->command) == 4 && strcmp("quit", m->command) == 0) {
            break;
        }
        handle_command(g, 0, m); // g0m. like the fyad guy
        free(m);
    }
    // TODO kill threads, close sockets
    int i;
    if (g->players > 1) {
        printf("You can hit ctrl-c now, or wait for other players to acknowledge the quit, if necessary\n");
    }
    for (i = 0; i < g->players - 1; ++i) {
        close(g->sockets[i]);
    }
    for (i = 0; i < g->players - 1; ++i) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

int play_single() {
    char *name = get_username();
    game g = new_game(1);
    g->names[0] = name;
    printf("Ready to begin!\n");
    play_game(g, NULL);
    return 0;
}

void *interact_with_remote(void *arg) {
    igstruct *ig = (igstruct*)arg;
    uint8_t i = ig->i;
    game g = ig->g;
    int sockfd = g->sockets[i];
    uint8_t player_num = i+1;
    message msg = new_msg(pnum_to_string(player_num), "That's you.");
    if (msg == NULL) {
        return NULL; // uh oh
    }
    send_message(g, player_num, msg);
    free(msg);
    msg = new_msg(cnum_to_string(g->player_indices[player_num]), "That's the character you currently control.");
    if (msg == NULL) {
        return NULL; // uh oh
    }
    send_message(g, player_num, msg);
    free(msg);
    char *buf = (char*)malloc(BUF_SIZE * sizeof(char));
    if (buf == NULL) {
        return NULL; // error
    }
    memset(buf, 0, BUF_SIZE);
    while (1) {
        char *inp = receive_input(sockfd, buf);
        if (inp == NULL) {
            break;
        }
        msg = parse_message(inp);
        free(inp);
        if (msg == NULL) {
            continue; // try again
        }
        handle_command(g, player_num, msg);
        destroy_message(msg);
    }
    printf("%s (%s) is out and you get their stuff.\n", g->names[player_num], pnum_to_string(player_num));
    return NULL;
}

int host_multi() {
    char *name = get_username();
    printf("How many total players? (2 to 4)\n");
    long num_players = 0;
    while (num_players < 2 || num_players > 4) {
        num_players = get_number_from_stdin();
    }
    uint8_t num = (uint8_t)num_players;
    game the_game = new_game(num);
    the_game->names[0] = name;
    // now find the rest of the players.
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr_storage other_addr;
    socklen_t addr_size;
    int status = 0;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    char *myhostname = (char*)malloc(BIGGISH_NUMBER * sizeof(char));
    gethostname(myhostname, BIGGISH_NUMBER);
    if ((status = getaddrinfo(NULL, MY_PORT, &hints, &servinfo)) != 0) {
        printf("couldn't getaddrinfo, so this isn't happening\n");
        return 1;
    }
    int sock;
    int b = -1;
    struct addrinfo *p;
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == -1) {
            continue;
        }
        b = bind(sock, p->ai_addr, p->ai_addrlen);
        if (b == 0) {
            break;
        }
        close(sock);
    }
    if (b != 0) {
        printf("Couldn't bind, so this isn't happening\n");
        return 1;
    }
    printf("Tell them to connect to %s\n", myhostname);
    listen(sock, BACKLOG);
    addr_size = sizeof(other_addr);
    uint8_t friends_connected = 1;
    while(friends_connected < num) {
        int new_fd = accept(sock, (struct sockaddr*)&other_addr, &addr_size);
        uint8_t code = 0;
        char *buf = (char*)malloc(BUF_SIZE*sizeof(char));
        memset(buf, 0, BUF_SIZE*sizeof(char));
        char *name = unline(receive_input(new_fd, buf));
        if (valid_name(name)) {
            int i;
            uint8_t broken = 0;
            for (i = 0; i < 4; ++i) {
                if (the_game->names[i] && strlen(the_game->names[i]) == strlen(name) && strcmp(the_game->names[i], name) == 0) {
                    code = 1; // name conflict
                    send(new_fd, &code, 1, 0);
                    close(new_fd);
                    free(buf); free(name); broken = 1; break;
                }
            }
            if (broken) {
                continue;
            }
            i = getpeername(sock, (struct sockaddr*)&other_addr, &addr_size);
            if (!i) {
                code = 4; // we had an error
                send(new_fd, &code, 1, 0);
                close(new_fd);
                free(buf); free(name); broken = 1; continue;
            }
            char *otherIP = (char*)malloc((1 + addr_size) * sizeof(char));
            if (!otherIP) {
                // TODO
            }
            if (!inet_ntop(p->ai_family, ((struct sockaddr*)&other_addr)->sa_data, otherIP, addr_size)) {
                code = 4;
                send(new_fd, &code, 1, 0);
                close(new_fd);
                free(buf); free(name); free(otherIP); continue;
            }
            printf("A player connecting from %s wants to join as \"%s\". Is this OK? (y/n)\n", otherIP, name);
            char *ans = NULL;
            i = 0;
            while (ans == NULL) {
                ans = unline(get_one_stdin_line());
                if (!strcmp("y", ans)) {
                    the_game->sockets[friends_connected - 1] = new_fd;
                    the_game->names[friends_connected] = strdup(name);
                    code = 0;
                    send(new_fd, &code, 1, 0);
                    ++friends_connected;
                    printf("ok, letting them in. We now have %u out of %u players.\n", friends_connected, num);
                }
                else if (!strcmp("n", ans)) {
                    printf("ok, rejecting them\n");
                    code = 2;
                    send(new_fd, &code, 1, 0);
                    close(new_fd);
                    free(buf); free(name); free(otherIP);
                    i = 1;
                }
                else {
                    printf("please say y or n\n");
                    ans = NULL;
                }
                free(ans);
            }
        }
        else {
            code = 3; // won't happen under normal operation
            send(new_fd, &code, 1, 0);
            close(new_fd);
            free(buf); free(name); continue;
        }
    }
    friends_connected = 0;
    for (b = 0; b < num - 1; ++b) {
        send(the_game->sockets[b], &friends_connected, 1, 0);
    }
    // TODO make threads
    pthread_t threads[num - 1];
    igstruct igs[num - 1];
    for (friends_connected = 0; friends_connected < num - 1; ++friends_connected) {
        igs[friends_connected].g = the_game;
        igs[friends_connected].i = friends_connected;
        pthread_create(&threads[friends_connected], NULL, interact_with_remote, (void*)&igs[friends_connected]);
    }
    play_game(the_game, threads);
    return 0;
}

void *output_loop(void *arg) {
    istruct *is = (istruct*)arg;
    int sockfd = is->i;
    char *buf = (char*)malloc(BUF_SIZE * sizeof(char));
    if (buf == NULL) {
        close(sockfd);
        return NULL;
    }
    memset(buf, 0, BUF_SIZE * sizeof(char));
    char *output = NULL;
    while (1) {
        output = receive_input(sockfd, buf);
        if (output == NULL) {
            break;
        }
        printf("%s\n", output);
        free(output);
    }
    printf("It looks like we've been disconnected. Maybe type a couple newlines?\n");
    return NULL;
}

int input_loop(pthread_t *output_thread, int sockfd) {
    char *input = NULL;
    while (1) {
        input = get_one_stdin_line();
        if (input == NULL) {
            continue;
        }
        send(sockfd, input, strlen(input), 0);
    }
    pthread_join(*output_thread, NULL);
    return 0;
}

int join_multi() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *servinfo;
//    struct sockaddr_storage other_addr;
//    socklen_t addr_size;
    int sockfd;
    int status = 0;
    // in a loop:
    char *server = NULL;
    char *name = NULL;
    uint8_t errCode;
    while(1) {
    //  ask for server
        printf("What's the computer you want to connect to?\n");
        while (server == NULL) {
            server = get_one_stdin_line();
        }
        if ((status = getaddrinfo(unline(server), MY_PORT, &hints, &servinfo)) != 0) {
            printf("That doesn't seem to be a valid server; let's try again?\n");
            free(server);
            server = NULL;
            continue;
        }
    //  in a loop:
        errCode = 0;
        while (errCode < 4) {
            sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
            if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) {
                printf("Got an error trying to connect. Maybe this isn't a good idea.\n");
                free(server);
                server = NULL;
                continue;
            }
            name = reline(get_username());
            send(sockfd, name, strlen(name), 0);
            recv(sockfd, &errCode, 1, 0);
            status = 0;
            if (errCode > 0) {
                free(name);
            }
            switch(errCode) {
                case 1:
                    printf("Your name conflicted with another on the server. You'll need to pick a new one.\n");
                    break;
                case 2:
                    printf("The owner of the server rejected you. It might be because of your name, but maybe not.\n");
                    printf("Do you want to keep trying this server? (y/n)\n");
                    name = NULL;
                    while (name == NULL) {
                        name = get_one_stdin_line();
                        if (strcmp("y\r\n", name) == 0) {
                        }
                        else if (strcmp("n\r\n", name) == 0) {
                            status = 1;
                        }
                        else {
                            printf("come on man give me a y or an n\n");
                            free(name); name = NULL;
                        }
                    }
                    free(name);
                    break;
                case 3:
                    printf("This should never happen\n");
                    break;
                case 4:
                    printf("Seems like that server had a technical problem. Let's try another one.\n");
                    break;
                default:
                    status = 2;
                    break;
                    // now actually join the game
            }
            if (status) {
                break;
            }
        }
        if (status == 2) {
            break;
        }
    }
    printf("We're in the game! Now hold tight until all the requisite players join.\n");
    recv(sockfd, &errCode, 1, 0);
    if (errCode) {
        printf("Looks like the server host decided not to do this. Oh well. :(\n");
        return 0;
    }
    else {
        printf("Alright, we've got enough players to go\n");
    }
    istruct is;
    is.i = sockfd;
    pthread_t output_thread;
    pthread_create(&output_thread, NULL, output_loop, &is);
    input_loop(&output_thread, sockfd);
    return 0;
}

int main(int argc, char* argv[]) {
    printf("Welcome to my game!\n");
    printf("To start with, do you want to\n\
    play a single-player game (s),\n\
    host a multi-player game (h), or\n\
    join a multi-player game (j)?\n");
    int nope = 1;
    while (nope) {
        char *reply = get_one_stdin_line();
        if (!reply) {
            return 1;
        }
        if (!strncmp(reply, "s", 1)) {
            // do something
            nope = 0;
            play_single();
        }
        if (!strncmp(reply, "h", 1)) {
            // do something else
            nope = 0;
            host_multi();
        }
        if (!strncmp(reply, "j", 1)) {
            // do something else
            nope = 0;
            join_multi();
        }
        if (nope) {
            printf("Please answer with either 's', 'h', or 'j' (followed by a newline...)\n");
        }
        free(reply);
    }
    return 0;
}
