#include "shell/module/common/pipe.h"
#include "shell/module/common/path.h"
#include "shell/module/common/redirect.h"
#include <pty.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <utmp.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>

extern char **environ;

/**
 * Pipeline Manager
 */
void extract_pipe(char **argv) {
	int cmd_start = 0;
	int prev_fd = -1;
	int child_count = 0;

	for (int i = 0; ; i++) {
		if (argv[i] == nullptr || strcmp(argv[i], "|") == 0) {
			int p_fds[2];
			bool has_next = (argv[i] != nullptr && strcmp(argv[i], "|") == 0);
			char **current_argv = &argv[cmd_start];
			char *saved_token = argv[i];
			argv[i] = nullptr;

			if (has_next) {
				if (pipe(p_fds) < 0) {
					perror("pipe");
					_exit(1);
				}
			}

			pid_t pid = fork();
			if (pid == 0) {
				/* --- Child process --- */
				if (prev_fd != -1) {
					dup2(prev_fd, STDIN_FILENO);
					close(prev_fd);
				}
				if (has_next) {
					close(p_fds[0]); // Close read end
					dup2(p_fds[1], STDOUT_FILENO);
					close(p_fds[1]);
				}

				handle_redirection(current_argv);
				char *resolved = resolve_path(current_argv[0]);
				if (resolved) {
					execve(resolved, current_argv, environ);
					free(resolved);
				}
				fprintf(stderr, "ShapeQuake: %s: command not found\n", current_argv[0]);
				_exit(127);
			}

			/* --- Parent (Manager) process --- */
			child_count++;
			if (prev_fd != -1) close(prev_fd);
			
			if (has_next) {
				close(p_fds[1]); // Close write end
				prev_fd = p_fds[0];
				argv[i] = saved_token;
				cmd_start = i + 1;
			} else {
				break;
			}
		}
	}
	for (int j = 0; j < child_count; j++) wait(nullptr);
	_exit(0);
}

