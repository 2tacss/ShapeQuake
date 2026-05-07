#include "shell/shell.h"
#include "protocol.h"
#include "shell/handlers.h"
#include "allocator.h"
#include "runtime/executer.h"
#include "runtime/net.h"
#include "error/sq_error.h"
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>



/**
 * Handles from handlers
*/
const sq_shell_ops_t g_default_shell_ops = {
	.handle_backspace = handle_backspace_default,
	.handle_char = handle_char_default,
	.execute = execute_default,
};


//extern const sq_shell_ops_t g_default_shell_ops;

SQ_NODISCARD
int sq_shell_init(sq_shell_t *shell, int rows, int cols) {
	/* Reset line buffer */
	memset(shell->line_buffer, 0, sizeof(shell->line_buffer));
	shell->line_len = 0;
	
	/* Set function table for shell operations */
	shell->ops = &g_default_shell_ops;

	/* Allocate and initialize the execution engine */
	shell->exec = (sq_executer_t *)sq_malloc(sizeof(sq_executer_t));
	if (shell->exec == nullptr) {
		SqErr.fatal(SqErrMsg.out_of_memory);
		return -1; 
	}
	shell->exec->master_fd = -1;
	shell->exec->child_pid = -1;
	shell->exec->is_running = false;
	shell->exec->read_thread = 0;

	/* 
	 * Initialize server connection.
	 * Attempt to connect once at startup. If it fails, fd is set to -1.
	 */
	int fd = get_server_connection();
	if (fd == -1) SqErr.fatal(SqErrMsg.unable_create_fd);
	shell->net_middleware.fd = fd;

	(void)rows;
	(void)cols;

	return 0;
}

void sq_shell_input_byte(sq_shell_t *shell, byte b) {
	if (shell == NULL || shell->ops == NULL) return;

	/* 
	 * Handle line execution on Carriage Return or Line Feed 
	 */
	if (b == '\r' || b == '\n') {
		// handler.c: execute_default()
		shell->ops->execute(shell);
	} 
	/* Handle Backspace or Delete (ASCII 0x7f/0x08) */
	else if (b == 0x7f || b == 0x08) {
		shell->ops->handle_backspace(shell);
	} 
	/* Standard character input */
	else {
		shell->ops->handle_char(shell, b);
	}
}

/**
 * Output a standard newline sequence.
 * This ensures compatibility across different terminal environments.
 */
void output_newline(sq_shell_t *shell) {
	(void)shell;
	/* \r\n is used to ensure the cursor moves to the start of the next line */
	printf("\r\n");
	fflush(stdout);
}

void sq_shell_finalize(sq_shell_t *shell) {
	if (shell == NULL) return;

	if (shell->net_middleware.fd != -1) {
		close(shell->net_middleware.fd);
		shell->net_middleware.fd = -1;
	}
	
	if (shell->exec != NULL) {
		sq_free(shell->exec);
		shell->exec = NULL;
	}
}
