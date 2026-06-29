#include "core/builtin.h"
#include "core/shell.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

extern volatile bool g_stop_required;
static int shell_builtin_cd(shell_t *shell, token_list_t *args);
static int shell_builtin_exit(shell_t *shell, token_list_t *args);

static int shell_builtin_cd(shell_t *shell, token_list_t *args) {
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

static int shell_builtin_exit(shell_t *shell, token_list_t *args) {
	(void)shell;
	(void)args;
	g_stop_required = true;
	return 0;
}

void shell_builtin_init_regystry(shell_t *shell) {
	shell->builtin_registry.count = 0;

	shell_builtin_register(
		shell, 
		"cd", 
		shell_builtin_cd, 
		"Change the current directory.", 
		BUILTIN_TYPE_STATIC, 
		nullptr
	);

	shell_builtin_register(
		shell, 
		"exit", 
		shell_builtin_exit, 
		"Exit ShapeQuake.", 
		BUILTIN_TYPE_STATIC, 
		nullptr
	);
}

bool shell_builtin_register(shell_t *shell, const char *name, shell_builtin_handler_t handler, const char *help, builtin_type_t type, void *dl_handle) {
	if (!shell || !name || !handler) return false;

	if (shell->builtin_registry.count >= SHELL_BUILTIN_MAX) {
		return false;
	}

	size_t idx = shell->builtin_registry.count;
	shell->builtin_registry.commands[idx].name = name;
	shell->builtin_registry.commands[idx].handler = handler;
	shell->builtin_registry.commands[idx].help = help;
	shell->builtin_registry.commands[idx].type = type;
	shell->builtin_registry.commands[idx].dl_handle = dl_handle;

	shell->builtin_registry.count++;
	return true;
}

bool shell_builtin_execute(shell_t *shell, token_list_t *tokens) {
    if (tokens == nullptr || tokens->count == 0) return false;
    const char *cmd_name = tokens->tokens[0];

    for (size_t i = 0; i < shell->builtin_registry.count; ++i) {
        if (strcmp(shell->builtin_registry.commands[i].name, cmd_name) == 0) {
            shell->builtin_registry.commands[i].handler(shell, tokens);
            return true;
        }
    }
    return false;
}
