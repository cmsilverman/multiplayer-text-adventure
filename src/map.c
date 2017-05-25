#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "utils.h"

map new_map() {
    map res = (map)malloc(sizeof(map_tile));
    if (res == NULL) {
        return NULL;
    }
    memset(res, 0, sizeof(map_tile));
    return res;
}

void destroy_map(map m) {
    if (m == NULL) {
        return;
    }
    free(m);
}

void set_neighbor(map m, map n, uint8_t direction) {
    m->neighbors[direction] = n;
}

void set_description(map m, char *desc) {
    if (strlen(desc) > INPUT_LENGTH) {
        return;
    }
    m->description = strdup(desc);
}
