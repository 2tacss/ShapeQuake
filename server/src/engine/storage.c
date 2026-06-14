#include <stdio.h>
#include <sqlite3.h>
#include "defines.h"
#include "engine/net.h"
#include "protocol.h"
#include "allocator.h"
#include "status.h"
#include "engine/storage.h"


bool require_db_context(sq_db_context_t *ptr) {
	if (ptr == nullptr) return false;
	return true;
}

bool sq_db_init(sq_db_context_t *ctx, char *db_path) {
	if (ctx == nullptr) return -1;

	
	int rc = sqlite3_open(db_path, &ctx->h_db);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(ctx->h_db));
		return false;
	}
	ctx->arena_overview_cnt = (arena_t *)arena_init(SIZE_BLOCK_DEFAULT);

	ctx->is_open = true;
	ctx->is_arena_req = true;
	arena_set_contains_db_connection(ctx->arena_overview_cnt, true);

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
		fprintf(stderr, "SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);
		return false;
	}

	return true;
}

void sq_db_close(sq_db_context_t *ctx) {
	if (ctx && ctx->is_open) {
		sqlite3_close(ctx->h_db);
		ctx->is_open = false;
		arena_set_contains_db_connection(ctx->arena_overview_cnt, false);
	}
}

status_t sq_db_close_context(sq_db_context_t *ctx, bool require_force_destroy) {
	if (ctx && ctx->is_arena_req) {
		return arena_destroy(ctx->arena_overview_cnt, require_force_destroy);
	}
	return asstatus(CAT_SERVER, CND_FAILURE, CODE_DESTROY);
}

/**
 * Main node Backlog data
 */
int sq_db_save_backlog(sq_server_context_t *ctx_server, const sq_overview_content_t *ctx, const char *cmd, const char *output) {
	if (!ctx_server->ctx_db.h_db) return -1;

	const char *sql = "INSERT INTO command_logs (timestamp, project, working_dir, command, output) VALUES (?, ?, ?, ?, ?);";
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(ctx_server->ctx_db.h_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(ctx_server->ctx_db.h_db));
		return -1;
	}

	sqlite3_bind_int64(stmt, 1, ctx->timestamp);
	sqlite3_bind_text(stmt, 2, ctx->project_name, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, ctx->working_dir, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, cmd, -1, SQLITE_STATIC);
	
	sqlite3_bind_text(stmt, 5, output ? output : "", -1, SQLITE_STATIC);

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(ctx_server->ctx_db.h_db));
	}

	sqlite3_finalize(stmt);
	return (rc == SQLITE_DONE) ? 0 : -1;
}

void sq_load_backlog(sq_server_context_t *ctx_server) {
	const char *sql = "SELECT id, timestamp, project, working_dir, command, output from command_logs;";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(ctx_server->ctx_db.h_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(ctx_server->ctx_db.h_db));
		return;
	}
		int id;
		int timestamp;
		const byte *project_name;
		const byte *working_dir;
		const byte *command;
		const byte *output;
		(void)id;
		(void)timestamp;
		(void)project_name;
		(void)working_dir;
		(void)command;
		(void)output;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		id = sqlite3_column_int(stmt, 0);
		timestamp = sqlite3_column_bytes(stmt, 1);
		project_name = sqlite3_column_text(stmt, 2);
		working_dir = sqlite3_column_text(stmt, 3);
		command = sqlite3_column_text(stmt, 4);
		output = sqlite3_column_text(stmt, 5);
	}
	sqlite3_finalize(stmt);
}
