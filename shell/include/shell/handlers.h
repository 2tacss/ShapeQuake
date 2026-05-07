#ifndef SHAPEQUAKE_HANDLERS_H
#define SHAPEQUAKE_HANDLERS_H

#include "shell/shell.h"

extern const sq_shell_ops_t g_default_shell_ops;
void sq_notify_middleware(sq_shell_t *shell, const char *output, size_t output_len);
void sq_handler_dispatch(sq_shell_t *shell, const char *line);

#endif
