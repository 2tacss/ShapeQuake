#ifndef SHAPEQUAKE_SHELL_H
#define SHAPEQUAKE_SHELL_H

#include "common.h"
#include "defines.h"
#include "protocol.h"
#include "runtime/executer.h"
#include "runtime/net.h"

typedef struct sq_shell_s sq_shell_t;

typedef struct {
	void (*handle_backspace)(sq_shell_t *shell);
	void (*handle_char)(sq_shell_t *shell, byte b);
	void (*execute)(sq_shell_t *shell);
} sq_shell_ops_t;

struct sq_shell_s {
	char line_buffer[SQ_LINE_BUF_SIZE];
	size_t line_len;
	const sq_shell_ops_t *ops;
	sq_executer_t *exec;
	sq_socket_handle_t net_middleware;
};

SQ_NODISCARD int sq_shell_init(sq_shell_t *shell, int rows, int cols);
void handle_char_default(sq_shell_t *shell, byte b);
void handle_backspace_default(sq_shell_t *shell);
void execute_default(sq_shell_t *shell);
void sq_shell_input_byte(sq_shell_t *shell, byte b);
void output_newline(sq_shell_t *shell);
void sq_shell_finalize(sq_shell_t *shell);

#endif
