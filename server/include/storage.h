#ifndef SQ_STORAGE_H
#define SQ_STORAGE_H

#include "protocol.h"

int db_init(const char *db_path);
int db_save_command(const sq_context_t *ctx, const char *cmd, const char *output);
void db_close(void);

#endif
