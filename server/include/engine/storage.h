#ifndef SQ_STORAGE_H
#define SQ_STORAGE_H

#include "protocol.h"
#include "allocator.h"
#include <sqlite3.h>

typedef struct server_context_t sq_server_context_t;

typedef struct sq_db_context {
	sqlite3 *h_db;
	bool is_open;
	sq_arena_t *arena_overview_cnt;
	bool is_arena_req;
} sq_db_context_t;

bool require_db_context(sq_db_context_t *ptr);
bool require_contexts(sq_db_context_t *ptr); // not implemented yet
bool reuire_db_resources(sq_db_context_t *ptr); // not implemented yet
bool sq_db_init(sq_db_context_t *ctx, char *db_path);
void sq_db_close(sq_db_context_t *ctx);
sq_u16_t sq_db_close_context(sq_db_context_t *ctx, bool require_force_destroy);
int sq_db_save_backlog(sq_server_context_t *ctx_server, const sq_overview_content_t *ctx, const char *cmd, const char *output);
void sq_load_backlog(sq_server_context_t *ctx_server);

#endif
