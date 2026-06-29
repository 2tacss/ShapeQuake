#ifndef SQ_CORE_CONTEXT_H
#define SQ_CORE_CONTEXT_H

#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdatomic.h>

/* State of the ShapeQuake node */
typedef enum {
	SHELL_STATE_IDLE,      /* Terminal Node: Waiting for command */
	SHELL_STATE_EXECUTING, /* Situation Node: Command running in PTY */
	SHELL_STATE_EXITING    /* Shutdown sequence */
} shell_state_t;

typedef struct {
	_Atomic shell_state_t state;
	_Atomic pid_t foreground_pid;
	struct termios original_termios; 
	bool is_raw;
	size_t output_buffer_size;
	char *last_command;
} shell_context_t;

/* State control functions */
void shell_context_init(shell_context_t *ctx);
void shell_context_set_state(shell_context_t *ctx, shell_state_t state);
void shell_sys_terminal_raw(shell_context_t *ctx);
void shell_sys_terminal_cooked(shell_context_t *ctx);

#endif
