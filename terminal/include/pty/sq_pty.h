#ifndef SQ_TERMINAL_SQ_PTY_H
#define SQ_TERMINAL_SQ_PTY_H

#include "common.h"
#include <sys/types.h>

typedef struct {
	int fd;        /* This must be named 'fd' */
	pid_t pid;     /* This must be named 'pid' */
} sq_pty_t;

sq_pty_t* sq_pty_create(void);
void sq_pty_destroy(sq_pty_t *pty);
int sq_pty_spawn(sq_pty_t *pty, char **argv);
void sq_pty_write_byte(sq_pty_t *pty, byte b);

#endif
