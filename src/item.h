#ifndef ITEM_H_
#define ITEM_H_

#include <stdint.h>

#include "message.h"

#define NUM_ITEM_TYPES 1
#define MAX_ROOM_ITEMS (MAX_PARAMS)

char *item_names[NUM_ITEM_TYPES];

char *itemn_to_string(uint8_t num);

#endif
