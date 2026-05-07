#ifndef SQ_SHELL_TOKENIZER_H
#define SQ_SHELL_TOKENIZER_H

#include <stddef.h>
#include "common.h"

typedef struct {
    char **tokens;
    size_t count;     // nuber of tokens
    size_t capacity;  // the size of array currency for realloc()
} sq_token_list_t;

/**
 * Tokenize input character.
 */
SQ_NODISCARD sq_token_list_t* sq_tokenize(const char *input);

/**
 * Free tokens.
 */
void sq_token_list_destroy(sq_token_list_t *list);

#endif
