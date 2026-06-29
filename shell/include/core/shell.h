/* include/shell/shell.h */
#ifndef SHELL_CORE_SHELL_H
#define SHELL_CORE_SHELL_H

#include <stddef.h>
#include "defines.h"
#include "protocol.h"
#include "runtime/context.h"
#include "runtime/executer.h"
#include "core/builtin.h"

typedef struct shell_t shell_t;

#define SHELL_COMMAND_LINE_BUF_SIZE 1024
#define PTY_BUF_SIZE  4096

typedef struct {
	void (*handle_backspace)(shell_t *shell);
	void (*handle_char)(shell_t *shell, char b);
	void (*execute)(shell_t *shell);
} shell_ops_t;

struct shell_t {
	char command_line_buffer[SHELL_COMMAND_LINE_BUF_SIZE];
	size_t command_line_len;
	const shell_ops_t *operations;
	shell_context_t ctx; // shell state
	shell_executer_t exec; // pty connection, context
	sq_socket_handle_t net_middleware;
	shell_builtin_registry_t builtin_registry;
};

int shell_init(shell_t *shell, int rows, int cols);
void shell_finalize(shell_t *shell);
void shell_input_byte(shell_t *shell, byte b);

#endif
