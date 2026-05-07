#ifndef SQ_STORAGE_H
#define SQ_STORAGE_H

#include "protocol.h"

typedef struct {
	int id;
	int timestamp;
	char *project;
	char *working_dir;
	char *command;
	char *output;
} sq_table_commands;

int sq_db_init(const char *db_path);
void sq_db_close(void);
int sq_db_save_backlog(const sq_context_t *ctx, const char *cmd, const char *output);

#endif
