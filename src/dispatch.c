#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "dispatch.h"
#include "game.h"
#include "handler.h"
#include "message.h"

/*
 * Number of handler entries in handlers[]
 * Do NOT include the default handler entry in here
 */
#define NUM_HANDLER_ENTRIES 7

typedef int (*handler_function)(game g, uint8_t p, message m);

struct handler_entry {
    char *name;
    handler_function func;
};

#define HANDLER_ENTRY(NAME) { #NAME, handle_ ## NAME }

struct handler_entry handlers[] = {
    // most basic stuff
    HANDLER_ENTRY(quit), // 1
    HANDLER_ENTRY(look), // 2

    // movement
    HANDLER_ENTRY(north), // 3
    HANDLER_ENTRY(south), // 4
    HANDLER_ENTRY(east), // 5
    HANDLER_ENTRY(west), // 6
    HANDLER_ENTRY(xyzzy), // 7

    /* default */
    HANDLER_ENTRY(ERR_UNKNOWNCOMMAND)
};

struct handler_entry find_handle(char *command) {
    int i;
    for (i = 0; i < NUM_HANDLER_ENTRIES; ++i) {
        if (!strcmp(handlers[i].name, command)) {
            return handlers[i];
        }
    }
    /* if nothing was found return the default */
    return handlers[i];
}

int handle_command(game g, uint8_t p, message m) {
    if (g == NULL || m == NULL) {
        return -1;
    }
    char *command = m -> command;
    /*
     * do nothing if there's no command
     * I would have done this in parsing but it caused weird problems
     */
    if (!command || strlen(command) == 0) {
        return -1;
    }
    struct handler_entry the_handle = find_handle(command);
    return the_handle.func(g, p, m);
}
