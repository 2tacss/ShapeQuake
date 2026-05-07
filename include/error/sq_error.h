#ifndef SQ_ERROR_H_
#define SQ_ERROR_H_

typedef struct {
	char *shell_is_null;
	char *unable_init_shell;
	char *out_of_memory;
	char *unable_create_fd;
} sq_error_msg_t;

typedef struct {
	void (*const fatal)(const char *msg);
	void (*const warn)(const char *msg);
} sq_error_interface_t;

extern const sq_error_msg_t SqErrMsg;
extern const sq_error_interface_t SqErr;

#endif
