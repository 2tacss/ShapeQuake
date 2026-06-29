#ifndef SHAPEQUAKE_HANDLERS_H
#define SHAPEQUAKE_HANDLERS_H

#include "core/shell.h"

extern const shell_ops_t g_default_shell_ops;
void shell_cb_handle_char_default(shell_t *shell, char b);
void shell_cb_handle_backspace_default(shell_t *shell);
void shell_cb_execute_default(shell_t *shell);

void shell_cb_bridge_on_exec_output(void *context, const char *data, size_t len);
void shell_notify_middleware(shell_t *shell, const char *output, size_t output_len);
void shell_handler_dispatch(shell_t *shell, const char *line);

#endif
