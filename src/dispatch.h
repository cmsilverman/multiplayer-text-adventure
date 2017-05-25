#ifndef DISPATCH_H_
#define DISPATCH_H_

#include <stdint.h>
#include "game.h"
#include "message.h"

int handle_command(game g, uint8_t p, message m);

#endif
