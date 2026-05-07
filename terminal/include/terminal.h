#ifndef SHAPEQUAKE_TERMINAL_H
#define SHAPEQUAKE_TERMINAL_H

#include "common.h"
#include <vterm.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

/**
 * sq_terminal_t represents the "body" of the terminal emulator.
 * It encapsulates PTY, libvterm state, and the I/O thread.
 */
typedef struct {
	int pty_master;
	pid_t child_pid;
	pthread_t read_thread;
	volatile bool is_running;
	void *vt;
	void *vts;
	uint32_t rows;
	uint32_t cols;
	void *user_data;
} sq_terminal_t;
 
/**
 * Creates and initializes a terminal instance.
 */
SQ_NODISCARD
sq_terminal_t* sq_terminal_create(uint32_t rows, uint32_t cols);

/**
 * Destroys the terminal and cleans up resources.
 */
void sq_terminal_destroy(sq_terminal_t *term);

/**
 * Spawns a process (e.g., shell) inside the terminal's PTY.
 */
SQ_NODISCARD
int sq_terminal_spawn(sq_terminal_t *term, char **argv);

/**
 * Writes a single byte to the PTY (user input to process).
 */
void sq_terminal_write_byte(sq_terminal_t *term, byte b);

#endif // SHAPEQUAKE_TERMINAL_H
