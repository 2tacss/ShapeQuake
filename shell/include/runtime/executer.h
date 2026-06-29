/* include/runtime/executer.h */
#ifndef SHELL_CORE_EXECUTOR_H
#define SHELL_CORE_EXECUTOR_H

#include "core/tokenizer.h"
#include "status.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <stddef.h>

/* ================================================================================== *
 *  EXECUTER: manage user command stream / pty space / bridge                         *
 *    The bridge among shell and pty process. The command line on user inputting are  *
 *    monitored async by `read_thread`. The commands are put in `history_buffer` as   *
 *    raw data (max buffer is 64Kb). Once the aync thread catched command one line,   *
 *    tokenizes with `tokenizer,h` and `parse.h` and creates subprocess with command  *
 *    passing away to pty process, and run command in pty space.                      *
 *                                                                                    *
 *    Bridge callback: defined shell/core/callbacks.h                                 *
 *       void shell_cb_bridge_on_exec_output(                                         *
 *                  void *context,                                                    *
 *                  const char *data,                                                 *
 *                  size_t len);                                                      *
 * ================================================================================== */

#define SHELL_EXEC_HISTORY_BUF_SIZE 65536

typedef struct {
	// PTY
	int master_fd;
	int slave_fd;
	pthread_t read_thread; // monitoring pty
	pid_t spawned_pid; // handled by read_thread()
	atomic_bool is_thread_running;

	char history_buffer[SHELL_EXEC_HISTORY_BUF_SIZE];
	size_t history_offset;

	// notify output event occurences to shell with updated shell_context_t
	void (*on_output)(void *shell_ptr, const char *data, size_t len);
	void *shell_context;
} shell_executer_t;

void shell_executer_init(shell_executer_t *exec, void *shell_ptr, void (*callback)(void *, const char *, size_t));
[[nodiscard]] status_t shell_executer_spawn(shell_executer_t *exec, token_list_t *list);
void shell_executer_kill(shell_executer_t *exec);
void shell_executer_finalize(shell_executer_t *exec);

#endif
