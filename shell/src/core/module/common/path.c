#include "core/module/common/path.h"
#include "core/shell.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <utmp.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>

/**
 * Helper to resolve command name to absolute path using PATH environment variable.
 * Returns a newly allocated string or nullptr if not found.
 */
char *resolve_path(const char *cmd) {
	/* If cmd already contains '/', treat it as a path */
	if (cmd[0] == '/' || cmd[0] == '.') {
		return strdup(cmd);
	}

	char *env_path = getenv("PATH");
	if (!env_path) return nullptr;

	char *path_copy = strdup(env_path);
	char *dir = strtok(path_copy, ":");
	char *resolved = nullptr;

	while (dir != nullptr) {
		char full_path[SHELL_COMMAND_LINE_BUF_SIZE];
		snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
		if (access(full_path, X_OK) == 0) {
			resolved = strdup(full_path);
			break;
		}
		dir = strtok(nullptr, ":");
	}

	free(path_copy);
	return resolved;
}

