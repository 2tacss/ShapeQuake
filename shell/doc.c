/* --- include/core/allocator.h --- */
#include "defines.h"
#include <bits/posix2_lim.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <pthread.h>

typedef struct sq_arena_block {
	struct sq_arena_block *next;
	size_t offset;
	size_t capacity;
	alignas(16) char data[];
} sq_arena_block_t;

typedef struct {
	sq_arena_block_t *head;
	sq_arena_block_t *current;
	size_t block_size;
} sq_arena_t;


/* --- include/core/context.h --- */

typedef enum {
	SQ_STATE_IDLE,      /* Terminal Node: Waiting for command */
	SQ_STATE_EXECUTING, /* Situation Node: Command running in PTY */
	SQ_STATE_EXITING    /* Shutdown sequence */
} sq_state_t;

typedef struct {
	sq_state_t state;
	pid_t foreground_pid;
	int master_fd;      /* PTY master file descriptor */
	struct termios orig_termios;
	bool is_raw;
	
	/* Metadata for ShapeQuake nodes */
	size_t output_buffer_size;
	char *last_command;
} sq_context_t;


/* --- include/core/executor.h --- */

typedef void (*sq_output_callback_t)(void *context, const char *data, size_t len);

typedef struct {
	int master_fd;
	pid_t child_pid;
	pthread_t read_thread;
	volatile bool is_running;
	/* Notification Hook */
	sq_output_callback_t on_output;
	void *callback_context;
} sq_executer_t;

typedef struct {
	sq_arena_t *arena;    /* Arena for temporary line storage */
	char *delimiter;      /* Heredoc delimiter (e.g., EOF) */
	size_t total_size;    /* Total size of accumulated content */
} sq_heredoc_t;


/* --- include/shell/shell.h --- */

typedef struct sq_shell_s sq_shell_t;

typedef struct {
	void (*handle_backspace)(sq_shell_t *shell);
	void (*handle_char)(sq_shell_t *shell, byte b);
	void (*execute)(sq_shell_t *shell);
} sq_shell_ops_t;

struct sq_shell_s {
	char line_buffer[MAX_SHELL_LINE];
	size_t line_len;
	const sq_shell_ops_t *ops;
	sq_executer_t *exec;
	int fd_middleware;
};


/* --- include/shell/tokenizer.h --- */

typedef struct {
	char **tokens;
	size_t count;     /* number of tokens */
	size_t capacity;  /* the size of array currency for realloc() */
} sq_token_list_t;


/* --- include/core/protocol.h --- */

typedef struct {
	char project_name[64];
	char working_dir[256];
	uint64_t timestamp;
} sq_context_msg_t; /* To avoid conflict with sq_context_t */

typedef struct {
	uint32_t magic;
	uint32_t type;
	size_t payload_size;
	sq_context_msg_t context;
} sq_header_t;

typedef struct {
	uint32_t flags;
	char shape_hint[32];
	size_t cmd_len;
	char command_line[];
} sq_payload_exec_t;

typedef struct {
	double percentage;
	size_t bytes_read;
	uint32_t line_count;
} sq_progress_t;

typedef struct {
	sq_progress_t progress;
	size_t chunk_size;
	char data[];
} sq_payload_result_t;
