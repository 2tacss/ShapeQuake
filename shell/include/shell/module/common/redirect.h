#ifndef SQ_REDIRECT_H_
#define SQ_REDIRECT_H_

#include "allocator.h"

/* Heredoc management using arena allocator */
typedef struct {
	sq_arena_t *arena;    /* Arena for temporary line storage */
	char *delimiter;      /* Heredoc delimiter (e.g., EOF) */
	size_t total_size;    /* Total size of accumulated content */
} sq_heredoc_t;


void handle_redirection(char **argv);
/* Initialize heredoc context with a dedicated arena */
sq_heredoc_t *sq_heredoc_init(const char *delimiter);
char* sq_heredoc_read_all(sq_heredoc_t *hd);
void sq_heredoc_finish(sq_heredoc_t *hd);

#endif
