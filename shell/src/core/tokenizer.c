#include "core/tokenizer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

constexpr static int initial_token_capacity = 8;

typedef struct {
    const char *start;
    size_t len;
} string_view_t;

static void internal_add_token(token_list_t *list, const char *start, size_t len) {
	if (len == 0) return;

	char *word = malloc(len + 1);
	if (!word) return;

	memcpy(word, start, len);
	word[len] = '\0';

	if (list->count + 1 >= list->capacity) {
		size_t new_cap = list->capacity * 2;
		char **new_tokens = (char **)realloc(list->tokens, sizeof(char*) * new_cap);
		if (new_tokens == nullptr) {
			free(word);
			return;
		}
		list->tokens = new_tokens;
		list->capacity = new_cap;
	}
	list->tokens[list->count++] = word;
}

/* Trim whitespace from the beginning and end of a string */
static string_view_t internal_trim_whitespace(const char *str) {
	if (!str) return (string_view_t){NULL, 0};

	while (isspace((unsigned char)*str)) str++;
	if (*str == '\0') return (string_view_t){str, 0};

	const char *end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;

	return (string_view_t){str, (size_t)(end - str + 1)};
}

token_list_t *shell_tokenize(const char *input) {
	if (!input) return nullptr;

	string_view_t sv = internal_trim_whitespace(input);
	if (sv.len < 1 || !sv.start) return nullptr;

	token_list_t *list = (token_list_t *)malloc(sizeof(token_list_t));
	if (!list) return nullptr;

	list->capacity = initial_token_capacity;
	list->count = 0;
	list->tokens = (char **)malloc(sizeof(char*) * list->capacity);
	if (list->tokens == nullptr) {
		free(list);
		return nullptr;
	}

	const char *p = sv.start;
	const char *end = sv.start + sv.len;
	while (p < end) {
		while (p < end && isspace((unsigned char)*p)) p++;
		if (p >= end) break;

		if (*p == '|') {
			internal_add_token(list, p, 1);
			p++;
			continue;
		}

		const char *start = p;
		while (p < end && !isspace((unsigned char)*p) && *p != '|') p++;
		internal_add_token(list, start, (size_t)(p - start));
	}
	if (list->count >= list->capacity) {
		char **new_tokens = (char **)realloc(list->tokens, sizeof(char*) * (list->capacity + 1));
		if (new_tokens) {
			list->tokens = new_tokens;
			list->capacity++;
		} else {
			for (size_t i = 0; i < list->count; i++) {
				free(list->tokens[i]);
			}
			free(list->tokens);
			free(list);
			return nullptr;
		}
	}
	list->tokens[list->count] = nullptr;
	return list;
}

void shell_destroy_token(token_list_t *list) {
	if (list == nullptr) return;
	
	for (size_t i = 0; i < list->count; i++) {
		free(list->tokens[i]);
	}
	free(list->tokens);
	free(list);
}
