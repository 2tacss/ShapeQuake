#ifndef SQ_SHELL_UI_H_
#define SQ_SHELL_UI_H_

#include <unistd.h>
#include "core/shell.h"

typedef struct sq_shell_s sq_shell_t;

void print_prompt(shell_t *shell);
void sq_ui_dispatch_char(char c);

#endif
