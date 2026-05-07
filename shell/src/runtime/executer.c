#include <pty.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <utmp.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>
#include "common.h"
#include "runtime/net.h"
#include "protocol.h"
#include "runtime/executer.h"
#include "allocator.h"
#include "shell/handlers.h"
#include "shell/module/common/pipe.h"
#include "shell/module/common/path.h"
#include "ui/ui.h"


/**
 * Callback pthread
 * Async read loop for PTY output.
 */
static void *_read_childpty(void *arg) {
	sq_executer_t *exec = (sq_executer_t *)arg;
	char c;
	/* Buffer to hold the output log */
	char *output_log = sq_malloc(65536);
	size_t log_idx = 0;

	if (!output_log) return nullptr;

	while (exec->is_running) {
		ssize_t n = read(exec->master_fd, &c, 1);
		if (n > 0) {
			/* Dispatch to UI layer */
			sq_ui_dispatch_char(c);

			/* Accumulate log for notification */
			if (log_idx < 65535) {
				output_log[log_idx++] = c;
			}
		} else if (n <= 0) {
			if (n < 0 && errno == EINTR) continue;
			break;
		}
	}

	/*
	* Execute callback if data exists and handler is registered
	* handlers.c:
	*	shell->exec->on_output = _on_exec_output_bridge;
	*	shell->exec->show_prompt = print_prompt;
	*	shell->exec->callback_context = shell;
	*/
	if (log_idx > 0 && exec->on_output) {
		output_log[log_idx] = '\0';
		exec->on_output(exec->callback_context, output_log, log_idx);
	}

	if (exec->show_prompt) {
		exec->show_prompt((sq_shell_t *)exec->callback_context);
	}

	sq_free(output_log);
	int status = 0;
	waitpid(exec->child_pid, &status, 0);
	exec->is_running = false;
	return nullptr;
}

//void sq_executer_process_output(sq_context_t *ctx, int sock_middleware) {
//	char buf[4096];
//	ssize_t n = read(ctx->master_fd, buf, sizeof(buf));
//
//	if (n <= 0) {
//		/* Handle process exit or error... */
//		return;
//	}
//
//	/* 1. Update cumulative stats in context */
//	ctx->output_buffer_size += n;
//	for (ssize_t i = 0; i < n; i++) {
//		if (buf[i] == '\n') ctx->line_count++;
//	}
//
//	/* 2. Heuristic Progress Calculation */
//	double current_progress = 0.0;
//	if (ctx->expected_size > 0) {
//		/* Case: Known size (e.g., cat file) */
//		current_progress = (double)ctx->output_buffer_size / ctx->expected_size;
//	} else {
//		/* Case: Unknown size (e.g., grep) 
//		 * Use a logarithmic or asymptotic curve to keep the line moving
//		 * but never hitting 100% until the process actually exits.
//		 */
//		current_progress = 1.0 - (100.0 / (100.0 + ctx->output_buffer_size));
//	}
//
//	/* 3. Build and Stream the packet */
//	size_t payload_total = sizeof(sq_payload_result_t) + n;
//	sq_payload_result_t *res = malloc(payload_total);
//	
//	res->progress.percentage = current_progress;
//	res->progress.bytes_read = ctx->output_buffer_size;
//	res->progress.line_count = ctx->line_count;
//	res->chunk_size = n;
//	memcpy(res->data, buf, n);
//
//	/* Send Header + Payload to Middleware */
//	sq_header_t header = {
//		.magic = SQ_MAGIC,
//		.type = SQ_TYPE_EXEC_RESULT,
//		.payload_size = payload_total,
//		.context = ctx->saved_context /* Inherited from shell */
//	};
//
//	write(sock_middleware, &header, sizeof(header));
//	write(sock_middleware, res, payload_total);
//
//	free(res);
//}

/**
 * Spawn command execution as child process
 */
SQ_NODISCARD
int sq_executer_spawn(sq_executer_t *exec, char **argv) {
	if (argv == nullptr || argv[0] == nullptr || exec == nullptr) return -1;

	/* 
	 * Use forkpty to create a new PTY and avoid SIGHUP/Terminal 
	 * conflicts between parent and children.
	 */
	exec->child_pid = forkpty(&exec->master_fd, nullptr, nullptr, nullptr);
	if (exec->child_pid == -1) {
		perror("forkpty");
		return -1;
	}

	if (exec->child_pid == 0) {
		/* Inside PTY slave: build and run the pipeline */
		extract_pipe(argv);
	}

	/* Parent process: start async output monitoring */
	exec->is_running = true;
	if (pthread_create(&exec->read_thread, nullptr, _read_childpty, exec) != 0) {
		// running flag managed in sq_read_childpty()
		exec->is_running = false;
		return -1;
	}
	pthread_detach(exec->read_thread);
	return 0;
}

void sq_executer_kill(sq_executer_t *exec) {
	exec->is_running = false;
	if (exec->child_pid > 0) {
		kill(exec->child_pid, SIGHUP);
	}
	if (exec->master_fd >= 0) {
		close(exec->master_fd);
	}
	pthread_join(exec->read_thread, nullptr);
}
