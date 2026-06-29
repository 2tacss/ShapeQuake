#ifndef SQ_SHELL_UI_H_
#define SQ_SHELL_UI_H_

#include <unistd.h>
#include "core/shell.h"

typedef struct sq_shell_s sq_shell_t;

#define SHELL_UI_WRITE_BUF_SIZE 1024
#define SHELL_UI_REQUIRE_NEWLINE true
#define SHELL_UI_REQUIRE_NEWLINE_NO false

void shell_ui_prompt(shell_t *shell);
void shell_ui_blankline(shell_t *shell);
void shell_ui_put_prompt(shell_t *shell, bool require_newline);
void shell_ui_dispatch_char(char c);
void shell_ui_flush(void);

#endif
