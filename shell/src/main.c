#include "common.h"
#include "shell/shell.h"
#include "error/sq_error.h"
#include "shell/handlers.h"
#include "ui/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>

/* 
 * Build target selection:
 * DEBUG: For testing on existing terminals like Alacritty.
 * DEPLOY: For integration within the custom ShapeQuake terminal.
 */

volatile bool g_stop_required = false;
static struct termios g_orig_termios;

void set_terminal_raw_mode(void) {
#ifdef DEBUG
	/* Manual raw mode setup for external terminals */
	struct termios raw;
	if (tcgetattr(STDIN_FILENO, &g_orig_termios) != 0) return;
	raw = g_orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#else
	/* DEPLOY: Assume parent terminal has configured the PTY */
#endif
}

void restore_terminal_mode(void) {
#ifdef DEBUG
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
#endif
}

int main(void) {
	sq_shell_t shell;

	set_terminal_raw_mode();
#ifdef DEBUG
	atexit(restore_terminal_mode);
#endif

	/* Initialize shell logic (buffer and ops) */
	if (sq_shell_init(&shell, 24, 70) != 0) SqErr.fatal(SqErrMsg.unable_init_shell);

	print_prompt(&shell);

	/* 
	 * Main 1-byte granularity loop.
	 * Interacts directly with STDIN (the PTY).
	 */
	while (!g_stop_required) {
		byte b;
		if (read(STDIN_FILENO, &b, 1) > 0) {
			sq_shell_input_byte(&shell, b);
		}
	}

	sq_shell_finalize(&shell);
	return 0;
}
