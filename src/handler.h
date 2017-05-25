#ifndef HANDLER_H_
#define HANDLER_H_

#include <stdint.h>
#include "game.h"
#include "message.h"

int handle_quit(game g, uint8_t p, message m);
int handle_commands(game g, uint8_t p, message m);

int handle_north(game g, uint8_t p, message m);
int handle_south(game g, uint8_t p, message m);
int handle_east(game g, uint8_t p, message m);
int handle_west(game g, uint8_t p, message m);
int handle_xyzzy(game g, uint8_t p, message m);

int handle_look(game g, uint8_t p, message m);
int handle_switch(game g, uint8_t p, message m);

int handle_ERR_UNKNOWNCOMMAND(game g, uint8_t p, message m);

#endif
