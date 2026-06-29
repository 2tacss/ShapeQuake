#include <stdio.h>
#include <sqlite3.h>
#include "defines.h"
#include "engine/net.h"
#include "protocol.h"
#include "allocator.h"
#include "status.h"
#include "test/test.h"
#include "engine/storage.h"


bool require_db_context(sq_db_context_t *ptr) {
	if (ptr == nullptr) return false;
	return true;
}

bool sq_db_init(sq_db_context_t *ctx, char *db_path) {
	if (ctx == nullptr) return false;
	int rc = sqlite3_open(db_path, &ctx->h_db);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(ctx->h_db));
		return false;
	}
	ctx->arena_record_cache = (arena_t *)arena_init(SIZE_BLOCK_DEFAULT);

	ctx->is_open = true;
	ctx->is_arena_req = true;
	arena_set_contains_db_connection(ctx->arena_record_cache, true);

	const char *sql = 
		"CREATE TABLE IF NOT EXISTS command_logs ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"timestamp INTEGER,"
		"project TEXT,"
		"working_dir TEXT,"
		"command TEXT,"
		"output TEXT"
		");";

	char *err_msg = NULL;
	rc = sqlite3_exec(ctx->h_db, sql, NULL, NULL, &err_msg);
	if (rc != SQLITE_OK) {
		debug_meta_t d = DEBUG_META(
			asstatus(CAT_DB, CND_FATAL, CODE_CONNECTION),
			"sqlite3_exec()", "Unable to connect to sqlite."
		);
		dbgmsg(&d);
		sqlite3_free(err_msg);
		return false;
	}

	return true;
}

void sq_db_close(sq_db_context_t *ctx) {
	if (ctx && ctx->is_open) {
		sqlite3_close(ctx->h_db);
		ctx->is_open = false;
		arena_set_contains_db_connection(ctx->arena_record_cache, false);
	}
}

status_t sq_db_close_context(sq_db_context_t *ctx, bool require_force_destroy) {
	if (ctx && ctx->is_arena_req) {
		return arena_destroy(ctx->arena_record_cache, require_force_destroy);
	}
	return asstatus(CAT_SERVER, CND_FAILURE, CODE_DESTROY);
}

/**
 * Main node Backlog data
 */
int sq_db_save_backlog(sq_server_context_t *ctx_server, const sq_db_record_t *record, const char *cmd, const char *output) {
    if (!ctx_server || !ctx_server->ctx_db.h_db || !record || !cmd) return -1;

    const char *sql = "INSERT INTO command_logs (timestamp, project, working_dir, command, output) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(ctx_server->ctx_db.h_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        debug_meta_t d = DEBUG_META(
            asstatus(CAT_DB, CND_FATAL, CODE_DB_INSERT),
            "sqlite3_prepare_v2()", "Unable to prepare insert statement for main node overview content."
        );
        dbgmsg(&d);

        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(ctx_server->ctx_db.h_db));
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, record->timestamp);
    sqlite3_bind_text(stmt, 2, record->project_name ? record->project_name : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, record->working_dir ? record->working_dir : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, cmd, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, output ? output : "", -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(ctx_server->ctx_db.h_db));
    }

    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

void sq_load_backlog(sq_server_context_t *ctx_server) {
	if (!ctx_server || !ctx_server->ctx_db.h_db) return;

	const char *sql = "SELECT id, timestamp, project, working_dir, command, output from command_logs;";
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(ctx_server->ctx_db.h_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(ctx_server->ctx_db.h_db));
		return;
	}

	size_t loaded_count = 0;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		int64_t timestamp = sqlite3_column_int64(stmt, 1);
		const unsigned char *project_name = sqlite3_column_text(stmt, 2);
		const unsigned char *working_dir = sqlite3_column_text(stmt, 3);
		const unsigned char *command = sqlite3_column_text(stmt, 4);
		const unsigned char *output = sqlite3_column_text(stmt, 5);

		sq_db_record_t *rec = (sq_db_record_t *)arena_alloc(ctx_server->ctx_db.arena_record_cache, sizeof(sq_db_record_t));
		if (!rec) {
			fprintf(stderr, "Arena out of memory during database load.\n");
			break;
		}

		rec->timestamp = timestamp;
		
		if (project_name) {
			size_t len = strlen((const char *)project_name) + 1;
			char *buf = (char *)arena_alloc(ctx_server->ctx_db.arena_record_cache, len);
			if (buf) { memcpy(buf, project_name, len); rec->project_name = buf; }
		} else { rec->project_name = ""; }

		if (working_dir) {
			size_t len = strlen((const char *)working_dir) + 1;
			char *buf = (char *)arena_alloc(ctx_server->ctx_db.arena_record_cache, len);
			if (buf) { memcpy(buf, working_dir, len); rec->working_dir = buf; }
		} else { rec->working_dir = ""; }

		printf("[LOAD] ID: %d, Project: %s, Cmd: %s\n", id, rec->project_name, command ? (const char *)command : "NULL");
		loaded_count++;
	}

	sqlite3_finalize(stmt);
	printf("[LOAD FINISHED] Total %zu logs loaded into arena.\n", loaded_count);
}
