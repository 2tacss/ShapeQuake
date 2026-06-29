#include "engine/net.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <errno.h>
#include "protocol.h"
#include "allocator.h"
#include "heap.h"
#include "test/test.h"
#include "engine/storage.h"
#include "engine/net.h"
#include "status.h"

static heap_tracker_t static_tracker = {0};

sq_server_context_t *sq_server_create(const char *db_path) {
    if (!db_path) return nullptr;

	tracker_init(&static_tracker);
    sq_server_context_t *ctx = (sq_server_context_t *)heap_alloc(&static_tracker, sizeof(sq_server_context_t));
    if (!ctx) return nullptr;

    memset(ctx, 0, sizeof(sq_server_context_t));
	__asm__ volatile("" : : : "memory");

    ctx->arena = arena_init(SIZE_ARENA_DEFAULT);
    if (!ctx->arena) {
        heap_free(&static_tracker, ctx);
        return nullptr;
    }
    ctx->is_arena_req = true;

    int rc = sqlite3_open(db_path, &ctx->ctx_db.h_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(ctx->ctx_db.h_db));
        arena_destroy(ctx->arena, true);
        heap_free(&static_tracker, ctx);
        return nullptr;
    }
    ctx->ctx_db.is_open = true;

    ctx->ctx_db.arena_record_cache = (arena_t *)arena_init(SIZE_BLOCK_DEFAULT);
    if (!ctx->ctx_db.arena_record_cache) {
        sqlite3_close(ctx->ctx_db.h_db);
        arena_destroy(ctx->arena, true);
        heap_free(&static_tracker, ctx);
        return nullptr;
    }
    ctx->ctx_db.is_arena_req = true;
    arena_set_contains_db_connection(ctx->ctx_db.arena_record_cache, true);

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
    rc = sqlite3_exec(ctx->ctx_db.h_db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        debug_meta_t d = DEBUG_META(
            asstatus(CAT_DB, CND_FATAL, CODE_CONNECTION),
            "sqlite3_exec()", "Unable to initialize command_logs table."
        );
        dbgmsg(&d);
        sqlite3_free(err_msg);
        
        arena_set_contains_db_connection(ctx->ctx_db.arena_record_cache, false);
        arena_destroy(ctx->ctx_db.arena_record_cache, true);
        sqlite3_close(ctx->ctx_db.h_db);
        arena_destroy(ctx->arena, true);
        heap_free(&static_tracker, ctx);
        return nullptr;
    }

    return ctx;
}

void sq_server_destroy(sq_server_context_t *ctx, bool require_force_destroy) {
    if (!ctx) return;

    if (ctx->h_sock.is_running && ctx->h_sock.fd > -1) {
        close(ctx->h_sock.fd);
        ctx->h_sock.is_running = false;
    }

    if (ctx->ctx_db.is_open && ctx->ctx_db.h_db) {
        sqlite3_close(ctx->ctx_db.h_db);
        ctx->ctx_db.is_open = false;
        
        if (ctx->ctx_db.arena_record_cache) {
            arena_set_contains_db_connection(ctx->ctx_db.arena_record_cache, false);
        }
    }

    if (ctx->ctx_db.arena_record_cache && ctx->ctx_db.is_arena_req) {
        arena_destroy(ctx->ctx_db.arena_record_cache, require_force_destroy);
        ctx->ctx_db.arena_record_cache = nullptr;
        ctx->ctx_db.is_arena_req = false;
    }

    if (ctx->arena && ctx->is_arena_req) {
        arena_destroy(ctx->arena, require_force_destroy);
        ctx->arena = nullptr;
        ctx->is_arena_req = false;
    }

    heap_free(&static_tracker, ctx);
}

void start_listening(sq_server_context_t *ctx_server) {
	if (ctx_server == nullptr) exit(EXIT_FAILURE);
	
	int server_fd;
	struct sockaddr_un addr;

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd == -1) {
		debug_meta_t d = DEBUG_META(
			asstatus(CAT_SERVER, CND_FAILURE, CODE_CONNECTION),
			"socket()",
			"Unable to create server socket."
		);
		dbgmsg(&d);
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SQ_SOCKET_PATH, sizeof(addr.sun_path) - 1);
	unlink(SQ_SOCKET_PATH);
	
	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
		close(server_fd);
		debug_meta_t d = DEBUG_META(
			asstatus(CAT_SERVER, CND_FAILURE, CODE_CONNECTION),
			"bind()",
			"Unable to resolve binding server socket."
		);
		dbgmsg(&d);
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 5) == -1) {
		close(server_fd);
		debug_meta_t d = DEBUG_META(
			asstatus(CAT_SERVER, CND_FAILURE, CODE_CONNECTION),
			"listen()",
			"Unable to start open server."
		);
		dbgmsg(&d);
		exit(EXIT_FAILURE);
	}

	ctx_server->h_sock.fd = server_fd;
	ctx_server->h_sock.is_running = true;
	arena_set_contains_fd(ctx_server->arena, true);
}

status_t sq_send_header(sq_server_context_t *ctx, int client_fd, sq_packet_header_t *header) {
	if (!header) return asstatus(CAT_SERVER, CND_ABORT, CODE_VALUE);
	(void)ctx;

	const unsigned char *ptr = (const unsigned char *)header;
	size_t total_sent = 0;
	size_t total_size = sizeof(sq_packet_header_t);

	while (total_sent < total_size) {
		ssize_t n = send(client_fd, ptr + total_sent, total_size - total_sent, 0);

		if (n > 0) {
			total_sent += n;
		} else if (n == 0) {
			status_t st = asstatus(CAT_SERVER, CND_DEAD, CODE_SEND);
			debug_meta_t d = DEBUG_META(
				st,
				"send()",
				"Connection closed by client during sending."
			);
			dbgmsg(&d);
			close(client_fd);
			return st;
		} else {
			if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				close(client_fd);
				return asstatus(CAT_RESPONSE, CND_RETRY, CODE_CONNECTION);
			}
			
			debug_meta_t d = DEBUG_META(
				asstatus(CAT_SERVER, CND_ABORT, CODE_SEND),
				"send()",
				"Send abort."
			);
			dbgmsg(&d);
			close(client_fd);
			return asstatus(CAT_SERVER, CND_FAILURE, CODE_SEND);
		}
	}

	return asstatus(CAT_SERVER, CND_SUCCESS, CODE_SEND);
}

status_t sq_recv_header(sq_server_context_t *ctx, int client_fd, sq_packet_header_t *recv_header) {
	if (!recv_header) return asstatus(CAT_SERVER, CND_ABORT, CODE_VALUE);

	(void)ctx;
	
	char *ptr = (char *)recv_header;
	size_t total_recieved = 0;
	size_t act = sizeof(sq_packet_header_t);

	while (total_recieved < act) {
		ssize_t n = recv(client_fd, ptr+total_recieved, act - total_recieved, 0);
		if (n > 0) {
			total_recieved += n;
		} else if (n == 0) {
			debug_meta_t d = DEBUG_META(
				asstatus(CAT_SERVER, CND_REFUSE, CODE_RECV),
				"recv()",
				"Connection refused by client."
			);
			dbgmsg(&d);
			close(client_fd);
			return asstatus(CAT_SERVER, CND_REFUSE, CODE_RECV);
		} else {
			if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				close(client_fd);
				return asstatus(CAT_RESPONSE, CND_RETRY, CODE_CONNECTION);
			}
		}
	}
	if (recv_header->magic != SQ_MAGIC) {
		debug_meta_t d = DEBUG_META(
			asstatus(CAT_SERVER, CND_INVALID, CODE_MAGIC),
			"Recieved packet headcer",
			"Wrong Magic."
		);
		dbgmsg(&d);
		return asstatus(CAT_SERVER, CND_INVALID, CODE_MAGIC);
	}

	return asstatus(CAT_SERVER, CND_SUCCESS, CODE_RECV);
}

status_t recv_packet_body(sq_server_context_t *ctx, sq_packet_body_t *body) {
	(void)ctx;
	(void)body;
	return asstatus(CAT_SERVER, CND_SUCCESS, CODE_RECV);
}

/**
 * Handle data from a shell client and persist to SQLite.
 */
void sq_handle_client_payload(sq_server_context_t *ctx, int client_fd) {
	sq_packet_header_t recv_header = {0};
	status_t st = sq_recv_header(ctx, client_fd, &recv_header);
	if (get_cnd(st) != CND_SUCCESS) {
		return;
	}

	if (recv_header.payload_size > 0) {
		char *payload = arena_alloc(ctx->arena, recv_header.payload_size);
		if (!payload) return;
		ssize_t total_recv = 0;
		while (total_recv < (ssize_t)recv_header.payload_size) {
			ssize_t r = recv(client_fd, payload + total_recv, recv_header.payload_size - total_recv, 0);
			if (r <= 0) break;
			total_recv += r;
		}
		if (total_recv == (ssize_t)recv_header.payload_size) {
			char *cmd_ptr = payload;
			size_t cmd_len = strlen(cmd_ptr);
			char *out_ptr = (cmd_len + 1 < recv_header.payload_size) ? payload + cmd_len + 1 : NULL;
			// TODO: logic: parse payload to sq_packet_body_t and save to db
			// sq_db_save_backlog(ctx_server, &recv_header.content, cmd_ptr, out_ptr);
			printf("[LOGGED] %s (Output: %zu bytes)\n", cmd_ptr, out_ptr ? strlen(out_ptr) : 0);
		}
		free(payload);
	}
}
