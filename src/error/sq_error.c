#include "error/sq_error.h"
#include "defines.h"
#include <stdio.h>
#include <stdlib.h>

static void call_fatal(const char *msg) {
	fprintf(stderr, "[FATAL] %s", msg);
	exit(EXIT_FAILURE);
}

static void call_warn(const char *msg) {
	(void)msg;
}

const sq_error_msg_t SqErrMsg = {
	.shell_is_null = "shell is null.\n",
	.unable_init_shell = "Unable to initialize shell\n",
	.out_of_memory = "Out of memory.\n",
	.unable_create_fd = "Unable to create connection.\n",
};

const sq_error_interface_t SqErr = {
	.fatal = call_fatal,
	.warn  = call_warn,
};
