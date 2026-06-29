#ifndef SQ_SHELL_TOKENIZER_H
#define SQ_SHELL_TOKENIZER_H

#include <stddef.h>

typedef struct {
	char **tokens;
	size_t count;     // number of tokens, not delicious, but nuber is
	size_t capacity;  // the size of array currency for realloc()
} token_list_t;

[[nodiscard]] token_list_t* shell_tokenize(const char *input);
void shell_destroy_token(token_list_t *list);

#endif
