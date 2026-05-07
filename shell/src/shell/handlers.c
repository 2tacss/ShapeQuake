#include "runtime/executer.h"
#include "defines.h"
#include "common.h"
#include "error/sq_error.h"
#include "runtime/net.h"
#include "shell/shell.h"
#include "shell/handlers.h"
#include "shell/tokenizer.h"
#include "ui/ui.h"
#include "protocol.h"
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
 * Bridge function to notify middleware through the shell context.
 */
static void _on_exec_output_bridge(void *context, const char *data, size_t len) {
	sq_shell_t *shell = (sq_shell_t *)context;
	sq_send_backlog(shell, data, len);
}

/**
 * Internal output abstraction.
 * We use printf for now to output to Alacritty during DEBUG.
 */
static void shell_output_byte(sq_shell_t *shell, byte b) {
	(void)shell;
	putchar(b);
	fflush(stdout);
}

static void shell_output_str(sq_shell_t *shell, const char *str) {
	while (*str) {
		shell_output_byte(shell, (byte)*str++);
	}
}

/* --- Handlers --- */
void handle_char_default(sq_shell_t *shell, byte b) {
	if (shell->line_len < SQ_LINE_BUF_SIZE - 1) {
		shell->line_buffer[shell->line_len++] = (char)b;
		shell->line_buffer[shell->line_len] = '\0';
		shell_output_byte(shell, b);
	}
}

void handle_backspace_default(sq_shell_t *shell) {
	if (shell->line_len > 0) {
		shell->line_len--;
		shell->line_buffer[shell->line_len] = '\0';
		/* Visual backspace for CLI: back, space, back */
		shell_output_str(shell, "\b \b");
	}
}

/**
 * Parse, Taokenize STDIN and Execute
 *	Called in sq_shell_input_byte() in main()
 */
void execute_default(sq_shell_t *shell) {
	if (shell == NULL) SqErr.fatal(SqErrMsg.shell_is_null);
	if (shell->line_len == 0) {
		output_newline(shell);
		print_prompt(shell);
		return;
	}

	/* Tokenize the input buffer */
	sq_token_list_t *list = sq_tokenize(shell->line_buffer);
	if (list == NULL || list->count == 0) {
		if (list) sq_token_list_destroy(list);
		output_newline(shell);
		print_prompt(shell);
		return;
	}

	output_newline(shell);

	/* Built-in: exit */
	if (strcmp(list->tokens[0], "exit") == 0) {
		g_stop_required = true;
		sq_token_list_destroy(list);
		return;
	}


	/* External command execution via executer engine */
	if (list->count > 0) {
		/* Inject callback and context before starting the thread */
		shell->exec->on_output = _on_exec_output_bridge;
		shell->exec->show_prompt = print_prompt;
		shell->exec->callback_context = shell;

		write(STDOUT_FILENO, shell->line_buffer, strlen(shell->line_buffer));
		if (sq_executer_spawn(shell->exec, list->tokens) == 0) {
			//pass
		}
	}

	/* Cleanup and reset buffer */
	sq_token_list_destroy(list);
	shell->line_len = 0;
	memset(shell->line_buffer, 0, sizeof(shell->line_buffer));
}
