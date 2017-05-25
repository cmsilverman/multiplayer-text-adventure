#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "message.h"
#include "utils.h"

/*
 * instantiate_message - allocates memory for a message
 */
message instantiate_message() {
    char** pars = (char**) malloc(MAX_PARAMS * sizeof(char*));
    message res = (message) malloc(sizeof(message_struct));
    if (pars == NULL || res == NULL) {
        return NULL;
    }
    memset(pars, 0, MAX_PARAMS * sizeof(char*));
    res -> prefix = NULL;
    res -> command = NULL;
    res -> params = pars;
    res -> num_params = 0;
    return res;
}

/*
 * new_msg - instantiates a new message, then sets its
 *           prefix and command according to the args
 */
message new_msg(char *prefix, char *command) {
    message new_message = instantiate_message();
    if (new_message) {
        new_message -> prefix = prefix;
        new_message -> command = command;
    }
    return new_message;
}

/*
 * free_message - frees the message pointer
 *                but without freeing any of its contents
 *                (in case we still need to use those)
 * to free the contents as well, use destroy_message
 */
void free_message(message m) {
    if (m == NULL) {
        return;
    }
    free(m);
}

/*
 * destroy_message - frees the message and desroyes contents
 * to only free the message pointer, use free_message
 */
void destroy_message(message m) {
    if (m == NULL) {
        return;
    }
    if (m -> prefix != NULL) {
        free(m -> prefix);
    }
    if (m -> command != NULL) {
        free(m -> command);
    }
    if (m -> params != NULL) {
        char **p = (m -> params);
        int i;
        for (i = 0; i < MAX_PARAMS; ++i) {
            if ( (*p) != NULL) {
                free(*p);
                ++p;
            }
        }
        free(m -> params);
    }
    free_message(m);
}

void set_prefix(message m, char *prefix) {
    m -> prefix = prefix;
}

void set_command(message m, char *command) {
    m -> command = command;
}

/*
 * num_params - returns the number of parameters in a message
 */
int num_params(message m) {
    return m->num_params;
}

/*
 * add_param - adds a parameter to the first open slot of m
 */
int add_param(message m, char *param) {
    if (m == NULL || m -> params == NULL || param == NULL) {
        return 0;
    }
    if (m->num_params == MAX_PARAMS) {
        return 0;
    }
    *(m->params + m->num_params) = param;
    ++(m->num_params);
    return 1;
}

/*
 * get_param - finds the (index)th param of m (USING 0 INDEXING)
 *             returns NULL if m doesn't have (index+1) params
 */
char *get_param(message m, int index) {
    if (m == NULL || m -> params == NULL) {
        return NULL;
    }
    if (index >= m->num_params) {
        return NULL;
    }
    return m->params[index];
}

char *final_param(message m) {
    if (m == NULL || m -> params == NULL) {
        return NULL;
    }
    if (m->num_params == 0) {
        return NULL;
    }
    return m->params[(m->num_params) - 1];
}

void remove_final_param(message m) {
    if (m == NULL || m -> params == NULL) {
        return;
    }
    if (m->num_params == 0) {
        return;
    }
    free(m->params[m->num_params - 1]);
    m->params[m->num_params - 1] = NULL;
}

void replace_final_param(message m, char *param) {
    remove_final_param(m);
    add_param(m, param);
}

/*
 * print_message - returns the string form of a message being printed
 *                 RETURNS NEWLY ALLOCATED MEMORY
 */
char *print_message(message m) {
    if (m == NULL) {
        return NULL;
    }
    char *buf = (char*)malloc(LINE_SIZE * sizeof(char));
    size_t written = 0;
    size_t sres;
    if (m -> prefix) {
        sres = adjust_snprintf(snprintf(buf, INPUT_LENGTH - 1,
        "%s: ", m -> prefix), INPUT_LENGTH - 1);
        written += sres;
    }
    sres = adjust_snprintf(snprintf(buf + written,
        INPUT_LENGTH - 1 - written, "%s", m -> command),
        INPUT_LENGTH - 1 - written);
    written += sres;
    if (m -> params) {
        int i;
        char **p = m -> params;
        for (i = 0; i < MAX_PARAMS; ++i) {
            if (*p) {
                sres = adjust_snprintf(snprintf(buf + written,
                    INPUT_LENGTH - 1 - written, " %s", *p),
                    INPUT_LENGTH - 1 - written);
                written += sres;
            }
            ++p;
        }
    }
    sprintf(buf + written, "\r\n");
    return buf;
}

/*
 * message_strlen - returns the string length of a param if printed as above
 *                  without actually allocating a string
 * helpful in determining if the message needs to be broken down somehow
 */
int message_strlen(message m) {
    if (m == NULL) {
        return 0;
    }
    int c = 0;
    if (m -> prefix) {
        c += (1 + strlen(m -> prefix) + 1);
    }
    c += strlen(m -> command);
    if (m -> params) {
        int i;
        char **p = m -> params;
        for (i = 0; i < MAX_PARAMS; ++i) {
            if (*p) {
                c += (1 + strlen(*p));
            }
            ++p;
        }
    }
    return c + 2; // include the "\r\n" at the end
}

/*
 * parse_message - parses a string into the above struct
 * assumes the string contains exactly a message
 *  at this point, including the \r\n (and a terminating '\0')
 * note that an end parameter preceded by a colon is always placed
 *  in the MAX_PARAMS-th position
 */
message parse_message(char *input) {
    if (strlen(input) == 2) {
        return NULL;
    }
    message res = instantiate_message();
    if (res == NULL) {
        return NULL;
    }
    char *tok = (char*)malloc(LINE_SIZE*sizeof(char));
    int command_written = 0;
    int mode = 0; // 1 if we're in a quote
    size_t index = 0;
    char *cur = input;
    while (1) {
        if (*cur == '\r') {
            // we're at the end, check the mode etc
            if (mode == 1) {
                free(tok);
            }
            else {
                if (index) {
                    *(tok + index) = 0;
                    if (command_written) {
                        add_param(res, strdup(tok));
                    }
                    else {
                        set_command(res, strdup(tok));
                        command_written = 1;
                    }
                }
            }
            break;
        }
        else if (*cur == ' ' && mode == 0) {
            if (index) {
                *(tok + index) = 0;
                if (command_written) {
                    add_param(res, strdup(tok));
                }
                else {
                    set_command(res, strdup(tok));
                    command_written = 1;
                }
                index = 0;
            }
        }
        else if (*cur == '"') {
            if (mode == 0) {
                if (index) {
                    *(tok + index) = 0;
                    if (command_written) {
                        add_param(res, strdup(tok));
                    }
                    else {
                        set_command(res, strdup(tok));
                        command_written = 1;
                    }
                }
                index = 1;
                *tok = '"';
            }
            if (mode == 1) {
                *(tok+index) = '"';
                *(tok+index+1) = 0;
                if (command_written) {
                    add_param(res, strdup(tok));
                }
                else {
                    set_command(res, strdup(tok));
                    command_written = 1;
                }
                index = 0;
            }
            mode = 1 - mode;
        }
        else {
            *(tok + index) = *cur;
            ++index;
        }
        ++cur;
    }
    if (!command_written) {
        free(res);
        return NULL;
    }
    return res;
}
