#include "defines.h"
#include "error/sq_error.h"
#include "runtime/executer.h"
#include "runtime/net.h"
#include "core/tokenizer.h"
#include "core/shell.h"
#include "status.h"
#include "test/test.h"
#include "ui/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

extern volatile bool g_stop_required;

void shell_cb_bridge_on_exec_output(void *context, const char *data, size_t len) {
	shell_t *shell = (shell_t *)context;
	shell_send_backlog(shell, data, len);
}

static void shell_cb_output_str(shell_t *shell, const char *str) {
	(void)shell;
	while (*str) {
		shell_ui_dispatch_char((char)*str++);
		shell_ui_flush();
	}
}

void shell_cb_handle_char_default(shell_t *shell, char b) {
	if (shell->command_line_len < SHELL_COMMAND_LINE_BUF_SIZE - 1) {
		shell->command_line_buffer[shell->command_line_len++] = (char)b;
		shell->command_line_buffer[shell->command_line_len] = '\0';
		shell_ui_dispatch_char((char)b);
		shell_ui_flush();
	}
}

void shell_cb_handle_backspace_default(shell_t *shell) {
	if (shell->command_line_len > 0) {
		shell->command_line_len--;
		shell->command_line_buffer[shell->command_line_len] = '\0';
		/* Visual backspace for CLI: back, space, back */
		shell_cb_output_str(shell, "\b \b");
	}
}

/**
 * Parse, Taokenize STDIN and Execute
 *	Called in shell_input_byte() in main()
 */
void shell_cb_execute_default(shell_t *shell) {
	if (!shell) return;

	/* Tokenize the input buffer */
	token_list_t *list = shell_tokenize(shell->command_line_buffer);
	if (list == nullptr || list->count == 0) {
		if (list) shell_destroy_token(list);
		shell_ui_put_prompt(shell, SHELL_UI_REQUIRE_NEWLINE);
		return;
	}

	/* Built-in: exit */
	if (strcmp(list->tokens[0], "exit") == 0) {
		g_stop_required = true;
		shell_destroy_token(list);
		return;
	}

	if (list->count > 0) {
		shell_ui_blankline(shell);
		/* Inject callback and context before starting the thread */
		shell->exec.on_output = shell_cb_bridge_on_exec_output;
		shell->exec.shell_context = shell;

		status_t st = shell_executer_spawn(&shell->exec, list);
		status_t cmp = asstatus(CAT_SHELL_PTY, CND_FAILURE, CODE_OPEN);
		if (cmp.raw == st.raw) {
			debug_meta_t d = DEBUG_META(st, "shell_executer_spawn", "Unable to open pty space.");
			dbgmsg(&d);
		}
	}

	shell_destroy_token(list);
	shell->command_line_len = 0;
	memset(shell->command_line_buffer, 0, sizeof(shell->command_line_buffer));
	__asm__ volatile("" : : : "memory");
}

void handler_dispatch(shell_t *shell, const char *line) {
	(void) shell; (void) line;
	return;
}
