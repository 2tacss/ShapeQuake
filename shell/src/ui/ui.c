#include "ui/ui.h"
#include "core/shell.h"
#include "runtime/context.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static char static_write_buf[SHELL_UI_WRITE_BUF_SIZE];
static size_t static_buf_idx = 0;
static char static_last_char = 0;

void shell_ui_prompt(shell_t *shell) {
	(void)shell;
	char *prompt = "sq> ";
	write(STDOUT_FILENO, prompt, strlen(prompt));
	fflush(stdout);
}

void shell_ui_blankline(shell_t *shell) {
	(void)shell;
	write(STDOUT_FILENO, "\r\n", 2);
	fflush(stdout);
}

void shell_ui_put_prompt(shell_t *shell, bool require_newline) {
	if (shell && shell_context_get_state(&shell->ctx) == SHELL_STATE_IDLE) {
		if (require_newline) {
			shell_ui_blankline(shell);
		} else {
			char cr = '\r';
			write(STDOUT_FILENO, &cr, 1);
		}

		shell_ui_prompt(shell);
	}
}
}
