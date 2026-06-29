#include "defines.h"
#include "error/sq_error.h"
#include "runtime/net.h"
#include "core/tokenizer.h"
#include "core/shell.h"
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


/**
 * Bridge callback for shell_t and shell_executer_t
 */
void shell_cb_bridge_on_exec_output(void *context, const char *data, size_t len) {
	shell_t *shell = (shell_t *)context;
	shell_send_backlog(shell, data, len);
}

static void shell_cb_output_byte(shell_t *shell, byte b) {
	(void)shell;
	putchar(b);
	fflush(stdout);
}

static void shell_cb_output_str(shell_t *shell, const char *str) {
	while (*str) {
		shell_cb_output_byte(shell, (byte)*str++);
	}
}

/* --- Handlers --- */
void shell_cb_handle_char_default(shell_t *shell, char b) {
	if (shell->command_line_len < SHELL_COMMAND_LINE_BUF_SIZE - 1) {
		shell->command_line_buffer[shell->command_line_len++] = (char)b;
		shell->command_line_buffer[shell->command_line_len] = '\0';
		shell_cb_output_byte(shell, b);
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

	if (shell->command_line_len < 1) {
		output_newline(shell);
		print_prompt(shell);
	}
	

	/* Tokenize the input buffer */
	token_list_t *list = shell_tokenize(shell->command_line_buffer);
	if (list == nullptr || list->count == 0) {
		if (list) shell_destroy_token(list);
		output_newline(shell);
		print_prompt(shell);
		return;
	}

	output_newline(shell);

	/* Built-in: exit */
	if (strcmp(list->tokens[0], "exit") == 0) {
		g_stop_required = true;
		shell_destroy_token(list);
		return;
	}


	/* External command execution via executer engine */
	if (list->count > 0) {
		/* Inject callback and context before starting the thread */
		shell->exec.on_output = shell_cb_bridge_on_exec_output;
		shell->exec.history_buffer = {0};
		shell->exec.shell_context = shell;

		write(STDOUT_FILENO, shell->command_line_buffer, shell->command_line_len);
		shell_executer_spawn(shell, list);
	}

	/* Cleanup and reset buffer */
	shell_destroy_token(list);
	shell->command_line_len = 0;
	memset(shell->command_line_buffer, 0, sizeof(shell->command_line_buffer));
	__asm__ volatile("" : : : "memory");
}

void handler_dispatch(shell_t *shell, const char *line) {
	(void) shell; (void) line;
	return;
}
