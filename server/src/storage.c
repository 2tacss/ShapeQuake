#include <stdio.h>
#include <sqlite3.h>
#include "protocol.h"
#include "storage.h"

static sqlite3 *db = NULL;

int db_init(const char *db_path) {
	int rc = sqlite3_open(db_path, &db);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
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
	rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);
		return -1;
	}

	return 0;
}


int db_save_command(const sq_context_t *ctx, const char *cmd, const char *output) {
	if (!db) return -1;

	const char *sql = "INSERT INTO command_logs (timestamp, project, working_dir, command, output) VALUES (?, ?, ?, ?, ?);";
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int64(stmt, 1, ctx->timestamp);
	sqlite3_bind_text(stmt, 2, ctx->project_name, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, ctx->working_dir, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, cmd, -1, SQLITE_STATIC);
	
	sqlite3_bind_text(stmt, 5, output ? output : "", -1, SQLITE_STATIC);

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);
	return (rc == SQLITE_DONE) ? 0 : -1;
}

void db_close(void) {
	if (db) sqlite3_close(db);
}
