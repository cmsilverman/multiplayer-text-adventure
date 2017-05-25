#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "map.h"
#include "message.h"
#include "utils.h"

game new_game(uint8_t players) {
    if (players < 1 || players > 4) {
        return NULL;
    }
    game res = (game)malloc(sizeof(game_struct));
    memset(res, 0, sizeof(game_struct));
    if (res == NULL) {
        return NULL;
    }
    res->map = new_map();
    if (res->map == NULL) {
        destroy_game(res); return NULL;
    }
    res->secret_map = new_map();
    if (res->secret_map == NULL) {
        destroy_game(res); return NULL;
    }
    memset(res->map->items, 1, 4); // put in some torches
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
