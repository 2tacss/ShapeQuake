#include <pty.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <utmp.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>
#include "allocator.h"
#include "shell/module/common/redirect.h"
 

/**
 * Handle redirection tokens (e,g, > '>') within the current command context.
 * This function is called inky inside the child process.
 */
void handle_redirection(char **argv) {
	int i = 0;
	while (argv[i] != nullptr) {
		char *t = argv[i];
		bool processed = false;

		/* Output Redirection: > and >> */
		if (t[0] == '>') {
			char *filename = argv[i + 1];
			if (filename) {
				/* Handle both truncate (>) and append (>>) */
				int flags = O_WRONLY | O_CREAT | (t[1] == '>' ? O_APPEND : O_TRUNC);
				int fd = open(filename, flags, 0644);
				if (fd >= 0) {
					dup2(fd, STDOUT_FILENO);
					close(fd);
				}
				processed = true;
			}
		}
		/* Input Redirection: < */
		else if (t[0] == '<' && t[1] == '\0') {
			char *filename = argv[i + 1];
			if (filename) {
				int fd = open(filename, O_RDONLY);
				if (fd >= 0) {
					dup2(fd, STDIN_FILENO);
					close(fd);
				}
				processed = true;
			}
		}

		if (processed) {
			/* Physical Shift: Remove operator and its argument from argv */
			int j = i;
			while (argv[j + 2] != nullptr) {
				argv[j] = argv[j + 2];
				j++;
			}
			argv[j] = nullptr;
			/* Re-check current index for the newly shifted token */
		} else {
			i++;
		}
	}

	/* 
	 * Note: Heredoc (<<) is removed from this logic.
	 */
}


/* Initialize heredoc context with a dedicated arena */
sq_heredoc_t *sq_heredoc_init(const char *delimiter) {
	sq_heredoc_t *hd = sq_malloc(sizeof(sq_heredoc_t));
	if (!hd) return nullptr;

	/* Create arena with 1KB block size */
	hd->arena = sq_arena_init(SQ_LINE_BUF_SIZE);
	hd->delimiter = sq_strdup(delimiter);
	hd->total_size = 0;
	return hd;
}

/* Read lines from stdin until delimiter and flatten into a single buffer */
char* sq_heredoc_read_all(sq_heredoc_t *hd) {
	char line[SQ_LINE_BUF_SIZE];
	size_t delim_len = strlen(hd->delimiter);
	
	while (fgets(line, sizeof(line), stdin)) {
		/* Precise delimiter check: must match exactly and be followed by newline or null */
		if (strncmp(line, hd->delimiter, delim_len) == 0 && 
		   (line[delim_len] == '\n' || line[delim_len] == '\r' || line[delim_len] == '\0')) {
			break;
		}

		size_t len = strlen(line);
		char *saved = sq_arena_alloc(hd->arena, len);
		if (saved) {
			memcpy(saved, line, len);
			hd->total_size += len;
		}
	}

	char *flat = sq_malloc(hd->total_size + 1);
	if (!flat) return nullptr;

	size_t current_pos = 0;
	sq_arena_block_t *b = hd->arena->head;
	while (b) {
		if (b->offset > 0) {
			memcpy(flat + current_pos, b->data, b->offset);
			current_pos += b->offset;
		}
		b = b->next;
	}
	flat[hd->total_size] = '\0';
	return flat;
}

/* Cleanup all heredoc resources including the arena */
void sq_heredoc_finish(sq_heredoc_t *hd) {
	if (!hd) return;
	/* Destroy all blocks in the arena at once */
	sq_arena_destroy(hd->arena, true);
	sq_free(hd->delimiter);
	sq_free(hd);
}
