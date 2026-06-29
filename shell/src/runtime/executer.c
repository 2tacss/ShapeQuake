#define _GNU_SOURCE
#include "test/test.h"
#include "runtime/executer.h"
#include "core/module/common/path.h"
#include "core/tokenizer.h"
#include "status.h"
#include <pty.h>
#include <utmp.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <string.h>
#include <stdatomic.h>
#include <errno.h>
#include <wait.h>

extern char **environ;

static void *shell_executer_read_thread(void *executer) {
	if (!executer) return nullptr;

	shell_executer_t *exec = (shell_executer_t *)executer;
	if (exec->master_fd < 0) return nullptr;

	char buf[4096];

	while (atomic_load(&exec->is_thread_running)) {
		struct pollfd pfd = { .fd = exec->master_fd, .events = POLLIN };
		int num_fds = 1;
		int ret = poll(&pfd, num_fds, 100);

		if (ret > 0 && (pfd.revents & POLLIN)) {
			ssize_t len = read(exec->master_fd, buf, sizeof(buf));
			if (len > 0) {
				exec->on_output(exec->shell_context, buf, len);
			}
			else if (len == 0) {
				break;
			} else {
				if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
			}
		} else if (ret < 0) {
			if (errno == EINTR) continue;
			break;
		}
		int status;
		pid_t result = waitpid(exec->spawned_pid, &status, WNOHANG);
		if (result > 0) {
			exec->spawned_pid = -1;
			break;
		}
	}

	shell_t *shell = (shell_t *)exec->shell_context;
	shell_context_set_state(&shell->ctx, SHELL_STATE_IDLE);
	return nullptr;
}

void shell_executer_init(shell_executer_t *exec, void *shell_ptr, void (*callback)(void *, const char *, size_t)) {
	if (!exec || !shell_ptr || !callback) return;

	memset(exec, 0, sizeof(shell_executer_t));
	__asm__ volatile("" : : : "memory");

	exec->master_fd = -1;
	atomic_init(&exec->is_thread_running, false);
	exec->shell_context = shell_ptr;
	exec->on_output = callback;
}

// Fork and parse command and monitoring it with pthread(waitpid).
status_t shell_executer_spawn(shell_executer_t *exec, token_list_t *list) {
	if (!exec || !list) return asstatus(CAT_SHELL_EXECUTER, CND_FAILURE, CODE_PARAM);

	shell_context_set_state((shell_context_t *)exec->shell_context, SHELL_STATE_EXECUTING);

	if (openpty(&exec->master_fd, &exec->slave_fd, nullptr, nullptr, nullptr) < 0) {
		return asstatus(CAT_SHELL_PTY, CND_FAILURE, CODE_OPEN);
	}

	pid_t pid = fork();
	if (pid == 0) {
		login_tty(exec->slave_fd);
		char *cmd_path = resolve_path(list->tokens[0]);
		if (cmd_path != nullptr) {
			execve(cmd_path, list->tokens, environ);
			int exec_err = errno; // TODO: pass to caller
			(void)exec_err;
			free(cmd_path);
		}
		_exit(1);
	} else if (pid > 0) {
		exec->spawned_pid = pid;
		close(exec->slave_fd);
		atomic_store(&exec->is_thread_running, true);
		pthread_create(&exec->read_thread, nullptr, shell_executer_read_thread, exec);
		int pthread_err = errno; // TODO: pass to caller
		(void)pthread_err;
	} else {
		int err = errno;
		debug_meta_t d = DEBUG_META(
			asstatus(CAT_SHELL_EXECUTER, CND_FAILURE, CODE_OPEN),
			"fork()",
			strerror(err)
		);
		dbgmsg(&d);
	}
	return asstatus(CAT_SHELL_EXECUTER, CND_SUCCESS, CODE_EXIT);
}

void shell_executer_cleanup(shell_executer_t *exec) {
    if (exec->spawned_pid > 0) {
        int status;
        if (waitpid(exec->spawned_pid, &status, WNOHANG) == 0) {
            kill(exec->spawned_pid, SIGTERM);
            waitpid(exec->spawned_pid, &status, 0);
        }
        exec->spawned_pid = -1;
	}
}
