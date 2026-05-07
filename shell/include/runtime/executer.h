#ifndef SQ_CORE_EXECUTOR_H
#define SQ_CORE_EXECUTOR_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>
#include "common.h"
#include "allocator.h"
#include "runtime/net.h"

typedef void (*sq_output_callback_t)(void *context, const char *data, size_t len);
typedef void (*sq_show_prompt_callback_t)(sq_shell_t *shell);

typedef struct {
	int master_fd;
	pid_t child_pid;
	pthread_t read_thread;
	volatile bool is_running;
	/* Notification Hook */
	sq_output_callback_t on_output;
	sq_show_prompt_callback_t show_prompt;
	void *callback_context;
} sq_executer_t;


/* Existing process functions */
SQ_NODISCARD int sq_executer_spawn(sq_executer_t *exec, char **argv);
void sq_executer_kill(sq_executer_t *exec);

#endif
