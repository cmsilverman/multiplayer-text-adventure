#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "item.h"

char *item_names[NUM_ITEM_TYPES] = {"torch"};

char *itemn_to_string(uint8_t num) {
    if (num == 0 || num > NUM_ITEM_TYPES) {
        return NULL;
    }
    return item_names[num - 1];
}

uint8_t itemname_to_num(const char *name) {
    uint8_t i = 0;
    int len = strlen(name);
    for (i = 0; i < NUM_ITEM_TYPES; ++i) {
        if (strlen(item_names[i]) == len && strcmp(item_names[i], name) == 0) {
            return i + 1;
        }
    }
    return 0;
}
