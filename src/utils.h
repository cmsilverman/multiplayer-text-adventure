#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include "game.h"
#include "message.h"

#define INPUT_LENGTH 512
#define LINE_SIZE (INPUT_LENGTH + 1)
#define LONGLINE_SIZE (INPUT_LENGTH + 2)
#define BUF_SIZE (INPUT_LENGTH + 3)

#define MAX_NAME_LEN 12

typedef struct {
    int i;
} istruct;

typedef struct {
    uint8_t i;
    game g;
} igstruct;

char *get_username();

long get_number_from_stdin();

int valid_name(char *name);

char *get_one_stdin_line();

size_t adjust_snprintf(size_t res, size_t lim);

char *receive_input(int sockfd, char* buf);

message receive_message(int sockfd, char *buf);

message stdin_message();

char *unline(char *line);

char *reline(char *line);

void send_message(game g, uint8_t p, message msg);

void broadcast_message(game g, message msg);

int kill_player(game g, uint8_t p);

char *pnum_to_string(uint8_t digit);

char *pnum_to_username(game g, uint8_t digit);

char *cnum_to_string(uint8_t digit);

uint8_t get_current_leader(game g, uint8_t player);

uint8_t get_chars_leader(game g, uint8_t character);

char *direction_from_num(uint8_t num);

#endif
