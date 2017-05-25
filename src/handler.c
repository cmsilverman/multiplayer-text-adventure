#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <pthread.h>

#include "game.h"
#include "handler.h"
#include "map.h"
#include "message.h"
#include "utils.h"

int handle_quit(game g, uint8_t p, message m) {
    return 0;
}

int handle_look(game g, uint8_t p, message m) {
    g->directions[p] = 0;
    if (strcmp("look", m->command)) {
        // TODO look for additional params or whatever0
    }
    if (g->map == NULL) {
        return 0;
    }
    message reply = new_msg(m->command, (g->map)->description);
    broadcast_message(g, reply);
    free(reply);
    return 0;
}

int attempt_move(game g, uint8_t p, message m, uint8_t direction) {
    g->directions[p] = direction;
    if (direction == 0) {
        return 0; // shouldn't happen
    }
    message reply = NULL;
    map starting_map = g->map;
    if (starting_map == g->secret_map) { // we're at the secret place
        g->directions[p] = 0;
        reply = new_msg(m->command, "There is nowhere to go from here. It's turtles all the way down.");
        if (reply == NULL) {
            return 2;
        }
        send_message(g, p, reply);
        free(reply);
        return 0;
    }
    if (direction != XYZZY) {
        if (starting_map->neighbors[direction - 1] == NULL) {
            reply = new_msg(m->command, "There is no exit to that direction.");
            if (reply == NULL) {
                return 2;
            }
        }
        if (starting_map->locked[direction - 1] == 1 && g->keys[p] == 0) {
            reply = new_msg(m->command, "That exit is locked and you don't have a key.");
            if (reply == NULL) {
                return 2;
            }
        }
        if (reply) {
            send_message(g, p, reply);
            free(reply);
            return 0;
        }
    }
    uint8_t i;
    for (i = 0; i < g->players; ++i) {
        if (!g->dqed[i] && g->directions[i] != direction) {
            if (direction == XYZZY) {
                reply = new_msg("xyzzy", "Nothing appears to be happening...");
                if (reply == NULL) {
                    return 2;
                }
            }
            else {
                reply = new_msg(m->command, "Attempting to travel in that direction.");
                if (reply == NULL) {
                    return 2;
                }
            }
            if (reply) {
                send_message(g, p, reply);
                free(reply);
            }
            return 0;
        }
    }
    if (direction == XYZZY) {
        g->map = g->secret_map;
        // TODO
        handle_look(g, p, m);
        return 0;
    }
    map new_map = starting_map->neighbors[direction - 1];
    // no more movement for now
    for (i = 0; i < g->players; ++i) {
        g->directions[i] = 0;
    }
    if (__atomic_compare_exchange_n(&g->map, &starting_map, new_map, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        handle_look(g, p, m);
        return 0;
    }
    return 0;
}

int handle_north(game g, uint8_t p, message m) {
    return attempt_move(g, p, m, NORTH);
}
int handle_south(game g, uint8_t p, message m) {
    return attempt_move(g, p, m, SOUTH);
}
int handle_east(game g, uint8_t p, message m) {
    return attempt_move(g, p, m, EAST);
}
int handle_west(game g, uint8_t p, message m) {
    return attempt_move(g, p, m, WEST);
}
int handle_xyzzy(game g, uint8_t p, message m) {
    return attempt_move(g, p, m, XYZZY);
}

int handle_ERR_UNKNOWNCOMMAND(game g, uint8_t p, message m) {
    message reply = new_msg(m->command, "I don't recognize that command.");
    if (reply == NULL) {
        return 2;
    }
    send_message(g, p, reply);
    free(reply);
    return 0;
}
