#include "protocol.h"
#include "core/shell.h"
#include "core/callbacks.h"
#include "runtime/context.h"
#include "runtime/executer.h"
#include "runtime/net.h"
#include "ui/ui.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>

/**
 * Handles from handlers
*/
const shell_ops_t g_default_shell_ops = {
	.handle_backspace = shell_cb_handle_backspace_default,
	.handle_char = shell_cb_handle_char_default,
	.execute = shell_cb_execute_default,
};

extern const shell_ops_t g_default_shell_ops;

[[nodiscard]]
int shell_init(shell_t *shell, int rows, int cols) {
	memset(shell, 0, sizeof(shell_t));
	__asm__ volatile("" : : : "memory");

	shell_executer_init(&shell->exec, shell, shell_cb_bridge_on_exec_output);
	shell_context_init(&shell->ctx);
	shell->operations = &g_default_shell_ops;
	
	/* 
	 * Initialize server connection.
	 * Attempt to connect once at startup. If it fails, fd is set to -1.
	 */
	// int fd = get_server_connection();
	// if (fd == -1) SqErr.fatal(SqErrMsg.unable_create_fd);
	// shell->net_middleware.fd = fd;

	(void)rows;
	(void)cols;

	return 0;
}

void shell_input_byte(shell_t *shell, byte b) {
	if (shell == nullptr || shell->operations == nullptr) return;

	if (b == '\r' || b == '\n') {
		shell->operations->execute(shell);
	} else if (b == 0x7f || b == 0x08) {
		shell->operations->handle_backspace(shell);
	} else {
		shell->operations->handle_char(shell, b);
	}
}

void shell_finalize(shell_t *shell) {
	if (shell == NULL) return;

	if (shell->net_middleware.fd != -1) {
		close(shell->net_middleware.fd);
		shell->net_middleware.fd = -1;
	}
}
