#include <stdlib.h>
#include <stdint.h>

#include "item.h"

char *item_names[NUM_ITEM_TYPES] = {"torch"};

char *itemn_to_string(uint8_t num) {
    if (num == 0 || num > NUM_ITEM_TYPES) {
        return NULL;
    }
    return item_names[num - 1];
}
