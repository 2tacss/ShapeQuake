#ifndef SQ_CORE_CONTEXT_H
#define SQ_CORE_CONTEXT_H

#include "defines.h"
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

/* State of the ShapeQuake node */
typedef enum {
	SQ_STATE_IDLE,      /* Terminal Node: Waiting for command */
	SQ_STATE_EXECUTING, /* Situation Node: Command running in PTY */
	SQ_STATE_EXITING    /* Shutdown sequence */
} sq_state_t;

typedef struct {
	sq_state_t state;
	pid_t foreground_pid;
	int master_fd;      /* PTY master file descriptor */
	struct termios orig_termios;
	bool is_raw;
	
	/* Metadata for ShapeQuake nodes */
	size_t output_buffer_size;
	char *last_command;
} sq_context_t;

/* State control functions */
void sq_context_init(sq_context_t *ctx);
void sq_context_set_state(sq_context_t *ctx, sq_state_t state);
void sq_sys_terminal_raw(sq_context_t *ctx);
void sq_sys_terminal_cooked(sq_context_t *ctx);

#endif
