#include "ui/ui.h"
#include "core/shell.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void print_prompt(shell_t *shell) {
	(void)shell;
	char *prompt = "sq> ";
	write(STDOUT_FILENO, prompt, strlen(prompt));
	fflush(stdout);
}

void sq_ui_dispatch_char(char c) {
	if (c == '\n') {
		char cr = '\r';
		write(STDOUT_FILENO, &cr, 1);
	}
	write(STDOUT_FILENO, &c, 1);
}
