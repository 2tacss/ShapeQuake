#include "core/shell.h"
#include "ui/ui.h"
#include "core/builtin.h"
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>

/* 
 * Build target selection:
 * DEBUG: For testing on existing terminals like Alacritty.
 * DEPLOY: For integration within the custom ShapeQuake terminal.
 */

// SHELL
volatile bool g_stop_required = false;
static struct termios g_orig_termios;

#define DEBUG 1

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
	set_terminal_raw_mode();
#ifdef DEBUG
	atexit(restore_terminal_mode);
#endif

	shell_t shell = {0};
	int ret = shell_init(&shell, 80, 24);
	shell_builtin_init_regystry(&shell);
	
	shell_ui_put_prompt(&shell, SHELL_UI_REQUIRE_NEWLINE);

	/* 
	 * Main 1-byte granularity loop.
	 * Interacts directly with STDIN (the PTY).
	 */
	while (!g_stop_required) {
		byte b;
		if (read(STDIN_FILENO, &b, 1) > 0) {
			shell_input_byte(&shell, b);
		} else {
			break;
		}
	}

	(void)ret;
	return 0;
}

