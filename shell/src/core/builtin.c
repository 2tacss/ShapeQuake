#include "core/builtin.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

extern volatile bool g_stop_required;

static int sq_builtin_cd(shell_t *shell, token_list_t *args) {
	(void)shell;
	if (args->count < 2) {
		fprintf(stderr, "cd: argyment wrong\n");
		return 1;
	}

	if (chdir(args->tokens[1]) != 0) {
		perror("cd");
		return 1;
	}
	return 0;
}

static int sq_builtin_exit(shell_t *shell, token_list_t *args) {
	(void)shell;
	(void)args;
	g_stop_required = true;
	return 0;
}

static const sq_builtin_cmd_t g_builtins[] = {
	{ .name = "cd",   .handler = sq_builtin_cd,   .help = "Change the current directory." },
	{ .name = "exit", .handler = sq_builtin_exit, .help = "Exit ShapeQuake." },
	{ .name = nullptr, .handler = nullptr,        .help = nullptr }
};

bool sq_builtin_execute(shell_t *shell, token_list_t *tokens) {
	if (tokens == nullptr || tokens->count == 0) {
		return false;
	}

	const char *cmd_name = tokens->tokens[0];

	for (size_t i = 0; g_builtins[i].name != nullptr; ++i) {
		if (strcmp(g_builtins[i].name, cmd_name) == 0) {
			g_builtins[i].handler(shell, tokens);
			return true;
		}
	}

	return false;
}
