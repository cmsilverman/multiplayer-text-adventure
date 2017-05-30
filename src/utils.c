#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>
#include <errno.h>

#include "item.h"
#include "map.h"
#include "utils.h"

long get_number_from_stdin() {
    long retVal = 0;
    while (!retVal) {
        char *res = get_one_stdin_line();
        if (!res) {
            return -1;
        }
        errno = 0;
        retVal = strtol(res, NULL, 0);
        free(res);
        if (errno) {
            continue;
        }
    }
    return retVal;
}

char *get_username() {
    printf("Please pick a name consisting of 1 to %d alphanumeric characters (dash, single quote and underscore OK too)\n", MAX_NAME_LEN);
    char *res = NULL;
    while (res == NULL) {
        res = get_one_stdin_line();
        if (!res) {
            return NULL;
        }
        int l = strlen(res);
        if (l < 2) {
            free(res);
            return NULL;
        }
        *(res + l - 2) = 0; // no "\r\n"
        if (!valid_name(res)) {
            printf("Try again, that name isn't valid.\n");
            free(res);
            res = NULL;
        }
    }
    return res;
}

int valid_name(char *name) {
    if (name == NULL) {
        return 0;
    }
    int len = strlen(name);
    if (len == 0 || len > MAX_NAME_LEN) {
        return 0;
    }
    char *chk = name;
    while (*chk) {
        if (*chk == '\r' && *(chk + 1) == '\n') { // end of the string
            break;
        }
        if (isalnum(*chk) || *chk == '_' || *chk == '\'' || *chk == '-') {
            ++chk;
        }
        else {
            return 0;
        }
    }
    return 1;
}

/*
 * get_one_stdin_line - MALLOCS MEMORY for a single line of input from stdin
 * the result terminates in "\r\n\0"
 */
char *get_one_stdin_line() {
    char *buf = (char*)malloc(BUF_SIZE);
    char *result = (char*)malloc(BUF_SIZE);
    size_t sofar = 0;
    while (1) {
        char *read_attempt = fgets(buf, BUF_SIZE, stdin);
        if (!read_attempt) {
            free(buf);
            free(result);
            return NULL;
        }
        int l = strlen(buf);
        if (!l) {
            free(buf);
            free(result);
            return NULL;
        }
        size_t to_write = (size_t)l;
        if (BUF_SIZE - sofar < to_write) {
            to_write = BUF_SIZE - sofar;
        }
        memcpy(result + sofar, buf, to_write);
        sofar += to_write;
        if (*(buf + l - 1) == '\n') {
            size_t r_index = 0;
            if (sofar - 1 > r_index) {
                r_index = sofar - 1;
            }
            if (r_index > INPUT_LENGTH) {
                r_index = INPUT_LENGTH;
            }
            *(result + r_index) = '\r';
            *(result + r_index + 1) = '\n';
            *(result + r_index + 2) = 0;
            free(buf);
            return result;
        }
    }
}

/*
 * I want my snprintf calls to tell me how many characters were printed
 *  rather than how many would have been printed
 */
size_t adjust_snprintf(size_t res, size_t lim) {
    if (res >= lim) {
        return lim-1; 
    }
    return res;
}

/*
 * get_garbage_input - reads in input from the socket until a "\r\n" comes
 *                     through, ignoring everything before it
 *                     Copies everything afterward into buf
 * buf[0] is assumed to be the last character of the (too long) input
 * this is relevant because if '\r' is the last character of input read,
 * and '\n' is the first character of the new input, it should be caught
 */
void get_garbage_input(int sockfd, char* buf) {
    memset(buf + sizeof(char), 0, LONGLINE_SIZE);
    recv(sockfd, buf + 1, LONGLINE_SIZE - 1, 0);
    size_t current_length = strlen(buf);
    char *message_end = strstr(buf, "\r\n");
    if (message_end != NULL) {
        size_t diff = message_end - buf + 2;
        buf = strncpy(buf, buf + diff, current_length - diff);
        memset(buf + (current_length - diff), 0, diff);
        return;
    }
    *buf = *(buf + current_length - 1);
    get_garbage_input(sockfd, buf);
}

/*
 * receive_input - reads a message from a socket
 * buf string used to track input across multiple recv calls
 *  LONGLINE_SIZE until a "\r\n" is found
 * assumes the socket is connected correctly
 * RETURNS NEWLY MALLOC'D MEMORY! REMEMBER TO FREE IT!
 */
char *receive_input(int sockfd, char* buf) {
    /* Check if there is a full message stored in buf already */
    size_t current_length = strlen(buf);
    char *current_msg_end = strstr(buf, "\r\n");
    if (current_msg_end != NULL) {
        size_t diff = current_msg_end - buf + 2;
        char *res = strndup(buf, diff);
        buf = strncpy(buf, buf + diff, current_length - diff);
        memset(buf + (current_length - diff), 0, diff);
        return res;
    }
    size_t bytes_received = recv(sockfd, buf + current_length,
        LONGLINE_SIZE - current_length, 0);
    if (!bytes_received || bytes_received == -1) {
        return NULL;
    }
    char *message_end = strstr(buf, "\r\n");
    current_length = strlen(buf);
    if (message_end == NULL && current_length == LONGLINE_SIZE) {
        char *res = strndup(buf, LONGLINE_SIZE);
        message_end = res + LONGLINE_SIZE - 2;
        *buf = *(message_end + 1);
        *message_end = '\r';
        *(message_end + 1) = '\n';
        get_garbage_input(sockfd, buf);
        return res;
    }
    return receive_input(sockfd, buf);
}

message receive_message(int sockfd, char* buf) {
    char *socket_message_str = receive_input(sockfd, buf);
    if (socket_message_str == NULL) {
        return NULL;
    }
    message res = parse_message(socket_message_str);
    free(socket_message_str);
    if (res == NULL) {
        return NULL;
    }
    return res;
}

char *unline(char *line) {
    if (line == NULL) {
        return NULL;
    }
    char *current_msg_end = strstr(line, "\r\n");
    if (current_msg_end != NULL) {
        *current_msg_end = 0;
    }
    return line;
}

char *reline(char *line) {
    int sz = strlen(line);
    *(line + sz) = '\r';
    return line;
}

void send_message(game g, uint8_t p, message msg) {
    if (g == NULL || p >= g->players || msg == NULL) {
        return;
    }
    char *toPrint = print_message(msg);
    if (toPrint == NULL) {
        return;
    }
    if (p == 0) {
        printf("%s", toPrint);
    }
    else {
        int ln = strlen(toPrint);
        send(g->sockets[p-1], toPrint, ln, 0);
    }
    free(toPrint);
}

void broadcast_message(game g, message msg) {
    if (g == NULL || msg == NULL) {
        return;
    }
    char *toPrint = print_message(msg);
    if (toPrint == NULL) {
        return;
    }
    int ln = strlen(toPrint);
    int8_t p;
    printf("%s", toPrint);
//    printf("DEBUG. number of players is %u\n", g->players);
    for (p = 1; p < g->players; ++p) {
//        printf("DEBUG: sending to a socket...\n");
//        printf("DEBUG: p = %u; socket is %d; toPrint is %s\n", p, g->sockets[p-1], toPrint);
        send(g->sockets[p-1], toPrint, ln, 0);
    }
    free(toPrint);
}

message stdin_message() {
    char *txt = get_one_stdin_line();
    message m = parse_message(txt);
    free(txt);
    return m;
}

int kill_player(game g, uint8_t p) {
    if (p == 0) {
        return -1; // too much to do
    }
    uint8_t i = __atomic_exchange_n(&g->dqed[p], 1, __ATOMIC_SEQ_CST);
    if (i) {
        return 1;
    }
    i = g->player_indices[p];
    while (g->leader[i] != i) {
        i = g->leader[i];
    }
    // give this person's characters to P1
    printf("DEBUG. i = %u\n", i);
    g->leader[i] = 0;
    return 0;
}

char *pnum_to_string(uint8_t digit) {
    switch(digit) {
        case 0:
            return "P1";
        case 1:
            return "P2";
        case 2:
            return "P3";
        case 3:
            return "P4";
        default:
            break;
    }
    return NULL;
}

char *pnum_to_username(game g, uint8_t digit) {
    if (digit > g->players) {
        return NULL;
    }
    return g->names[digit];
}

char *cnum_to_string(uint8_t digit) {
    switch(digit) {
        case 0:
            return "Green";
        case 1:
            return "Red";
        case 2:
            return "Blue";
        case 3:
            return "Pink";
        default:
            break;
    }
    return NULL;
}

uint8_t get_current_leader(game g, uint8_t player) {
    uint8_t res = g->player_indices[player];
    while (g->leader[res] != res) {
        res = g->leader[res];
    }
    return res;
}

uint8_t get_chars_leader(game g, uint8_t character) {
    uint8_t res = character;
    while (g->leader[res] != res) {
        res = g->leader[res];
    }
    return res;
}

char *direction_from_num(uint8_t num) {
    switch(num) {
        case NORTH: 
            return "north";
        case SOUTH:
            return "south";
        case EAST:
            return "east";
        case WEST:
            return "west";
        default:
            break;
    }
    return NULL;
}
