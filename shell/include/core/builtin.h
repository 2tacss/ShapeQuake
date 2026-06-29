#ifndef SHAPEQUAKE_BUILTIN_H_
#define SHAPEQUAKE_BUILTIN_H_

#include <stdbool.h>
#include "core/shell.h"
#include "core/tokenizer.h"

typedef struct shell_builtin_cmd_t sq_builtin_cmd_t;
typedef int (*shell_builtin_handler_t)(shell_t *shell, token_list_t *args);

struct shell_builtin_cmd_t {
	const char *name;
	shell_builtin_handler_t handler;
	const char *help;
};

bool sq_builtin_execute(shell_t *shell, token_list_t *tokens);

#endif /* SHAPEQUAKE_BUILTIN_H_ */
