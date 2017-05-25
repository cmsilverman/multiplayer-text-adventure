#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>
#include "map.h"

typedef struct {
    volatile uint8_t leader[4]; // leader[i] = active player controlling i
    volatile map map;
    map secret_map;
    uint8_t players; // how many players
    uint8_t dqed[4]; // did this player leave the game? (0 by default)
    int sockets[3]; // actually size (players-1)
    /*
     * player_indices is the initially active character for each player
     * it continues to be useful in general through the leader field
     * most of these fields only actually have as many entries as players
     */
    uint8_t player_indices[4];
    volatile int8_t directions[4]; // 0 for not moving, which it usually is
    char *names[4];
    volatile uint8_t keys[4]; // number of keys per player. maybe capped at one?
} game_struct, *game;

game new_game(uint8_t players);

void destroy_game(game g);

#endif
