#ifndef _SQ_UI_UI_H
#define _SQ_UI_UI_H

#include <unistd.h>

typedef struct sq_shell_s sq_shell_t;

void print_prompt(sq_shell_t *shell);
void sq_ui_dispatch_char(char c);

#endif
