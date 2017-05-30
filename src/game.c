#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "item.h"
#include "map.h"
#include "message.h"
#include "utils.h"

map hardcoded_map() {
    map res = new_map();
    if (res == NULL) {
        return NULL;
    }
    map to_north = new_map();
    if (to_north == NULL) {
        destroy_map(res);
        return NULL;
    }
    map to_east = new_map();
    if (to_east == NULL) {
        destroy_map(res);
        destroy_map(to_north);
        return NULL;
    }
    map to_ne = new_map();
    if (to_ne == NULL) {
        destroy_map(res);
        destroy_map(to_north);
        destroy_map(to_east);
        return NULL;
    }
    set_neighbor(res, to_east, EAST);
    set_neighbor(to_east, res, WEST);
    set_neighbor(res, to_north, NORTH);
    set_neighbor(to_north, res, SOUTH);
    set_neighbor(to_east, to_ne, NORTH);
    set_neighbor(to_north, to_ne, EAST);
    set_neighbor(to_ne, to_east, SOUTH);
    set_neighbor(to_ne, to_north, WEST);
    memset(res->items, 1, 4);
    res->dark = 1;
    to_east->dark = 1;
    to_north->dark = 1;
    to_ne->dark = 1;
    return res;
}

game new_game(uint8_t players) {
    if (players < 1 || players > 4) {
        return NULL;
    }
    game res = (game)malloc(sizeof(game_struct));
    memset(res, 0, sizeof(game_struct));
    if (res == NULL) {
        return NULL;
    }
    res->map = hardcoded_map();
    if (res->map == NULL) {
        destroy_game(res); return NULL;
    }
    res->secret_map = new_map();
    if (res->secret_map == NULL) {
        destroy_game(res); return NULL;
    }
//    memset(res->map->items, 1, 4); // put in some torches
//    res->map->dark = 1; // oh shit!
    res->secret_map->description = "Now you stand atop the world turtle. To your north, east, south, and west are four elephants, upon the backs of which a gigantic flat disc is supported. There is nowhere to go from here. It's turtles all the way down.";
    res->players = players;
    switch(players) {
        case 2:
            res->player_indices[1] = 2;
            res->leader[2] = 2;
            res->leader[3] = 2;
            break;
        case 3:
            res->player_indices[1] = 2;
            res->player_indices[2] = 3;
            res->leader[2] = 2;
            res->leader[3] = 3;
            break;
        case 4:
            res->player_indices[1] = 1;
            res->player_indices[2] = 2;
            res->player_indices[3] = 3;
            res->leader[1] = 1;
            res->leader[2] = 2;
            res->leader[3] = 3;
            break;
        default:
            break;
    }
    memset((void*)(res->items), NOITEM, 4);
    return res;
}

void destroy_game(game g) {
    if (!g) {
        return;
    }
    if (g->map) {
        destroy_map(g->map);
    }
    if (g->names) {
        uint8_t i;
        for (i = 0; i < g->players; ++i) {
            if (g->names[i]) {
                free(g->names[i]);
            }
        }
        free(g->names);
    }
    free(g);
}
