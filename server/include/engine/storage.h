#ifndef SQ_STORAGE_H
#define SQ_STORAGE_H

#include "protocol.h"
#include <sqlite3.h>

typedef struct server_context_t sq_server_context_t;

typedef struct sq_db_context {
	sqlite3 *h_db;
	bool is_open;
} sq_db_context_t;


typedef struct {
	int id;
	int timestamp;
	char *project;
	char *working_dir;
	char *command;
	char *output;
} sq_table_commands;

bool sq_db_init(sq_server_context_t *ctx_server, char *db_path);
void sq_db_close(sq_server_context_t *ctx_server);
int sq_db_save_backlog(sq_server_context_t *ctx_server, const sq_overview_context_t *ctx, const char *cmd, const char *output);
void sq_load_backlog(sq_server_context_t *ctx_server);

#endif
