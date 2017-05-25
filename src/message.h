/*
 * Header file for messages
 * I'm not sure how much of this will be necessary but I can always cut parts
 *  out of it. tbh I am very stupid and don't know how to use modules     -Cody
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#define MAX_PARAMS 15

/*
 * message - a struct for irc messages
 * contains all of the information they can
 */
typedef struct message_struct {
    char* prefix; // optional; may be NULL
    char* command;
    char** params;
    int num_params;
} message_struct, *message;

message instantiate_message();
message new_msg(char *prefix, char *command);

void free_message(message m);
void destroy_message(message m);

void set_prefix(message m, char *prefix);
void set_command(message m, char *command);

int num_params(message m);
int add_param(message m, char *param);
char *get_param(message m, int index);
char *final_param(message m);

/* remove_final_param - removes the last parameter in a message
 * helpful for reusing the message struct for long messages
 */
void remove_final_param(message m);
/*
 * replace_final_param - replaces the last parameter in a message
 *                       with a new one
 * helpful for large strings of messages, or large messages needing
 *  to be sent across several strings of MAX_MESSAGE_LENGTH length
 */
void replace_final_param(message m, char *param);

char *print_message(message m);
int message_strlen(message m);

message parse_message(char *input);

#endif
