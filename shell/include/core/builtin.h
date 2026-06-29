#ifndef SHELL_BUILTIN_H_
#define SHELL_BUILTIN_H_

#include <stdbool.h>
#include "core/tokenizer.h"

typedef struct shell_t shell_t; 
typedef struct shell_builtin_cmd_t shell_builtin_cmd_t;
typedef struct shell_builtin_registry_t shell_builtin_registry_t;
typedef int (*shell_builtin_handler_t)(shell_t *shell, token_list_t *args);

#define SHELL_BUILTIN_MAX 128

typedef enum {
    BUILTIN_TYPE_STATIC,
    BUILTIN_TYPE_DYNAMIC
} builtin_type_t;

struct shell_builtin_cmd_t {
    const char *name;
    shell_builtin_handler_t handler;
    const char *help;
    builtin_type_t type;
    void *dl_handle; // dynamic lib
};

struct shell_builtin_registry_t {
    shell_builtin_cmd_t commands[SHELL_BUILTIN_MAX];
    size_t count;
};

void shell_builtin_init_regystry(shell_t *shell);
bool shell_builtin_register(shell_t *shell, const char *name, shell_builtin_handler_t handler, const char *help, builtin_type_t type, void *dl_handle);
bool shell_builtin_execute(shell_t *shell, token_list_t *tokens);

#endif /* SHAPEQUAKE_BUILTIN_H_ */
