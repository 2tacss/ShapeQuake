#include <stdio.h>
#include <sqlite3.h>
#include "protocol.h"
#include "allocator.h"
#include "engine/storage.h"

static sqlite3 *h_db = NULL;

int sq_db_init(const char *db_path) {
	int rc = sqlite3_open(db_path, &h_db);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(h_db));
		return -1;
	}

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
	rc = sqlite3_exec(h_db, sql, NULL, NULL, &err_msg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);
		return -1;
	}

	return 0;
}

void db_close(void) {
	if (h_db) sqlite3_close(h_db);
}

/**
 * Main node Backlog data
 */
int sq_db_save_backlog(const sq_context_t *ctx, const char *cmd, const char *output) {
	if (!h_db) return -1;

	const char *sql = "INSERT INTO command_logs (timestamp, project, working_dir, command, output) VALUES (?, ?, ?, ?, ?);";
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(h_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(h_db));
		return -1;
	}

	sqlite3_bind_int64(stmt, 1, ctx->timestamp);
	sqlite3_bind_text(stmt, 2, ctx->project_name, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, ctx->working_dir, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, cmd, -1, SQLITE_STATIC);
	
	sqlite3_bind_text(stmt, 5, output ? output : "", -1, SQLITE_STATIC);

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(h_db));
	}

	sqlite3_finalize(stmt);
	return (rc == SQLITE_DONE) ? 0 : -1;
}

void sq_load_backlog() {
	const char *sql = "SELECT id, timestamp, project, working_dir, command, output from command_logs;";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(h_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(h_db));
		return;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		int timestamp = sqlite3_column_bytes(stmt, 1);
		const byte *project = sqlite3_column_text(stmt, 2);
		const byte *working_dir = sqlite3_column_text(stmt, 3);
		const byte *command = sqlite3_column_text(stmt, 4);
		const byte *output = sqlite3_column_text(stmt, 5);
	}
	sqlite3_finalize(stmt);
}
