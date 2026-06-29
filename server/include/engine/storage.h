#ifndef SQ_SERVER_ENGINE_STORAGE_H_
#define SQ_SERVER_ENGINE_STORAGE_H_

#include "protocol.h"
#include "allocator.h"
#include <sqlite3.h>

typedef struct server_context_t sq_server_context_t;
typedef struct sq_db_record_t sq_db_record_t;
typedef struct sq_db_context_t sq_db_context_t;

#define DB_PATH "./overview.db"

struct sq_db_record_t {
    uint32_t id;
    uint32_t timestamp;
    const char *command;
    const char *working_dir;
    const char *project_name;
    /* 必要ならここにDB専用のメタデータ（タグとか）を足すぽよ */
};

struct sq_db_context_t {
	sqlite3 *h_db;
	bool is_open;
	arena_t *arena_record_cache;
	bool is_arena_req;
};

bool require_db_context(sq_db_context_t *ptr);
bool require_contexts(sq_db_context_t *ptr); // not implemented yet
bool reuire_db_resources(sq_db_context_t *ptr); // not implemented yet
bool sq_db_init(sq_db_context_t *ctx, char *db_path);
void sq_db_close(sq_db_context_t *ctx);
status_t sq_db_close_context(sq_db_context_t *ctx, bool require_force_destroy);
int sq_db_save_backlog(sq_server_context_t *ctx_server, const sq_db_record_t *record, const char *cmd, const char *output);
void sq_load_backlog(sq_server_context_t *ctx_server);

#endif
