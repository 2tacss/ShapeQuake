#include "shell/tokenizer.h"
#include "allocator.h"
#include <string.h>
#include <ctype.h>

constexpr static int initial_token_capacity = 8;

static void add_token(sq_token_list_t *list, const char *start, size_t len) {
	if (len == 0) return;

	char *word = sq_malloc(len + 1);
	if (word == nullptr) return;

	memcpy(word, start, len);
	word[len] = '\0';

	if (list->count + 1 >= list->capacity) {
		size_t new_cap = list->capacity * 2;
		char **new_tokens = (char **)sq_realloc(list->tokens, sizeof(char*) * new_cap);
		if (new_tokens == nullptr) {
			sq_free(word);
			return;
		}
		list->tokens = new_tokens;
		list->capacity = new_cap;
	}
	list->tokens[list->count++] = word;
}

/* Trim whitespace from the beginning and end of a string */
static char *sq_trim_whitespace(char *str) {
	if (str == NULL) return NULL;

	while (isspace((unsigned char)*str)) str++;
	if (*str == 0) return str;

	char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	end[1] = '\0';

	return str;
}

sq_token_list_t *sq_tokenize(const char *input) {
	if (input == nullptr) return nullptr;

	char *work_buf = sq_strdup(input);
	char *trimmed_input = sq_trim_whitespace(work_buf);

	if (trimmed_input == NULL || *trimmed_input == '\0') {
		sq_free(work_buf);
		return nullptr;
	}

	sq_token_list_t *list = (sq_token_list_t *)sq_malloc(sizeof(sq_token_list_t));
	if (list == nullptr) {
		sq_free(work_buf);
		return nullptr;
	}

	list->capacity = initial_token_capacity;
	list->count = 0;
	list->tokens = (char **)sq_malloc(sizeof(char*) * list->capacity);
	if (list->tokens == nullptr) {
		sq_free(list);
		sq_free(work_buf);
		return nullptr;
	}

	const char *p = trimmed_input;
	while (*p) {
		while (*p && isspace((unsigned char)*p)) {
			p++;
		}
		if (*p == '\0') break;

		if (*p == '|') {
			add_token(list, p, 1);
			p++;
			continue;
		}

		const char *start = p;
		while (*p && !isspace((unsigned char)*p) && *p != '|') {
			p++;
		}
		add_token(list, start, (size_t)(p - start));
	}

	list->tokens[list->count] = nullptr;
	sq_free(work_buf);

	return list;
}

void sq_token_list_destroy(sq_token_list_t *list) {
	if (list == nullptr) return;
	
	for (size_t i = 0; i < list->count; i++) {
		sq_free(list->tokens[i]);
	}
	sq_free(list->tokens);
	sq_free(list);
}
