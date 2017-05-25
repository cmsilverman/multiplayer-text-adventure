#ifndef MAP_H_
#define MAP_H_

#include <stdint.h>

#define NO_MOVE 0
#define NORTH 1
#define EAST 2
#define SOUTH 3
#define WEST 4
#define XYZZY 5

typedef struct map_tile {
//    // at least 4, could be more
//    uint8_t num_exits;
//    // sizes of all these are the same as num_exits
    /*
     * I'm holding off on that shit for now
     */
    struct map_tile *neighbors[4]; // null for not-an-exit
    volatile uint8_t locked[4]; // if the door currently needs a key
//    volatile uint8_t open[4]; // if people can go through the door at all
    char *description;
} map_tile, *map;

map new_map();

void destroy_map(map m);

void set_neighbor(map m, map n, uint8_t direction);

// you better believe this uses strdup
void set_description(map m, char *desc);

#endif
