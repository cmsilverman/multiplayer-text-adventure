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
#include "item.h"
#include "map.h"
#include "message.h"
#include "utils.h"

int handle_quit(game g, uint8_t p, message m) {
    if (p == 0) {
        return 0;
    }
    if (!kill_player(g, p)) {
        close(g->sockets[p-1]);
        message reply = new_msg(pnum_to_string(p), "*quits the game*");
        if (reply == NULL) {
            return 2;
        }
        broadcast_message(g, reply);
        free(reply);
    }
    return 0;
}

int handle_commands(game g, uint8_t p, message m) {
    char *body = "quit, commands, shout, look, north, south, east, west, switch, take, drop";
    message reply = new_msg(m->command, body);
    if (reply == NULL) {
        return 2;
    }
    send_message(g, p, reply);
    free(reply);
    return 0;
}

int handle_look(game g, uint8_t p, message m) {
    g->directions[p] = 0;
    if (strcmp("look", m->command)) {
        // TODO look for additional params or whatever0
    }
    map the_map = g->map;
    if (the_map == NULL) {
        return 0;
    }
    message reply;
    uint8_t room_was_dark = 0;
    uint8_t it;
    if (the_map->dark == 1) {
        room_was_dark = 1;
        for (it = 0; it < 4; ++it) {
            if (g->items[it] == 1) { // torch
                room_was_dark = 0; break;
            }
        }
        if (room_was_dark == 1) {
            reply = new_msg(m->command, "The room is dark and you can't see much.");
            for (it = 0; it < MAX_ROOM_ITEMS; ++it) {
                uint8_t the_item = the_map->items[it];
                if (the_item == 0) {
                    break;
                }
                if (the_item == 1) { // torch
                    add_param(reply, "However, you CAN see a torch on the ground...");
                    break;
                }
            }
            send_message(g, p, reply);
            free(reply);
        }
    }
    if (room_was_dark == 0) {
        reply = new_msg(m->command, the_map->description);
        if (reply == NULL) {
            return 2;
        }
        send_message(g, p, reply);
        free(reply);
        reply = new_msg(NULL, "items:");
        if (reply == NULL) {
            return 2;
        }
        uint8_t *item = the_map->items;
        for (it = 0; it < MAX_ROOM_ITEMS; ++it) {
            if (*item == 0) {
                break;
            }
            if (*item != NUM_ITEM_TYPES + 1) {
                add_param(reply, itemn_to_string(*item));
            }
            ++item;
        }
        send_message(g, p, reply);
        free(reply);
    }
    reply = new_msg(NULL, "exits:");
    if (reply == NULL) {
        return 2;
    }
    for (it = 0; it < 4; ++it) {
        map neighb = the_map->neighbors[it];
        if (neighb != NULL) {
            add_param(reply, direction_from_num(it + 1));
        }
    }
    send_message(g, p, reply);
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
        for (i = 0; i < g->players; ++i) {
            handle_look(g, i, m);
        }
        return 0;
    }
    map new_map = starting_map->neighbors[direction - 1];
    // no more movement for now
    for (i = 0; i < g->players; ++i) {
        g->directions[i] = 0;
    }
    if (__atomic_compare_exchange_n(&g->map, &starting_map, new_map, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        for (i = 0; i < g->players; ++i) {
            handle_look(g, i, m);
        }
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

int handle_switch(game g, uint8_t p, message m) {
    uint8_t start = get_current_leader(g, p);
    uint8_t next = start + 1;
    if (next == 4) {
        next = 0;
    }
    while (next != start) {
        if (get_chars_leader(g, next) == start) {
            g->leader[next] = next;
            g->leader[start] = next;
            break;
        }
        ++next;
        if (next == 4) {
            next = 0;
        }
    }
    message reply = new_msg(m->command, cnum_to_string(next));
    if (reply == NULL) {
        return 2;
    }
    send_message(g, p, reply);
    return 0;
}

int handle_take(game g, uint8_t p, message m) {
    message reply;
    if (num_params(m) < 1) {
        reply = new_msg(m->command, "You need to tell me which item to take");
        if (reply == NULL) {
            return 2;
        }
        send_message(g, p, reply);
        free(reply);
        return 0;
    }
    map the_map = g->map;
    uint8_t character = get_current_leader(g, p);
    uint8_t my_item = g->items[character];
    if (my_item == 0) {
        printf("We encountered a bug with %s having item 0\n", cnum_to_string(character));
        return 1; // this is a bug
    }
    uint8_t item_to_take = itemname_to_num(get_param(m, 0));
    if (item_to_take == 0) {
        reply = new_msg(m->command, "That's not the name of an item...");
        if (reply == NULL) {
            return 2;
        }
        send_message(g, p, reply);
        free(reply);
        return 0;
    }
    uint8_t i; uint8_t expected;
    for (i = 0; i < MAX_ROOM_ITEMS; ++i) {
        expected = the_map->items[i];
        if (expected == 0) {
            break;
        }
        if (expected == item_to_take) {
            if (__atomic_compare_exchange_n(&the_map->items[i], &expected, my_item, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                g->items[character] = expected;
                reply = new_msg(m->command, "Taken");
                if (reply == NULL) {
                    return 2;
                }
                if (my_item != NOITEM) {
                    add_param(reply, "(Dropped your");
                    add_param(reply, itemn_to_string(my_item));
                    add_param(reply, "in order to take it)");
                }
                send_message(g, p, reply);
                free(reply);
                return 0;
            }
        }
    }
    reply = new_msg(m->command, "Unable to find that item");
    if (reply == NULL) {
        return 2;
    }
    send_message(g, p, reply);
    free(reply);
    return 0;
}

int handle_drop(game g, uint8_t p, message m) {
    map the_map = g->map;
    message reply;
    uint8_t character = get_current_leader(g, p);
    uint8_t my_item = g->items[character];
    uint8_t i = 0;
    for (i = 0; i < MAX_ROOM_ITEMS; ++i) {
        uint8_t expected = the_map->items[i];
        if (expected == 0 || expected == NOITEM) {
            if (__atomic_compare_exchange_n(&the_map->items[i], &expected, my_item, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
                g->items[character] = -1;
                reply = new_msg(m->command, "Successfully dropped your held item");
                if (reply == NULL) {
                    return 2;
                }
                send_message(g, p, reply);
                free(reply);
                return 0;
            }
        }
    }
    reply = new_msg(m->command, "Unable to drop your item in this room");
    if (reply == NULL) {
        return 2;
    }
    send_message(g, p, reply);
    free(reply);
    return 0;
}

int handle_shout(game g, uint8_t p, message m) {
    message reply;
    if (num_params(m) == 0) {
        reply = new_msg(pnum_to_username(g, p), "*screams incoherently*");
        if (reply == NULL) {
            return 2;
        }
        broadcast_message(g, reply);
        free(reply);
        return 0;
    }
    reply = new_msg(pnum_to_username(g, p), get_param(m, 0));
    if (reply == NULL) {
        return 2;
    }
    broadcast_message(g, reply);
    free(reply);
    return 0;
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
