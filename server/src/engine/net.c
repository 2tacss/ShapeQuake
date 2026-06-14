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
#include "engine/storage.h"
#include "engine/net.h"
#include "status.h"

sq_server_context_t *sq_server_init_context(void) {
	/** require free() context **/
	size_t size_ctx = sizeof(sq_server_context_t);
	sq_server_context_t *ctx = (sq_server_context_t *)malloc(size_ctx);

	volatile char *ptr = (volatile char *)ctx;
	for (size_t i = 0; i < size_ctx; i++) {
		ptr[i] = 0;
	}

	// TODO: SQ_ARENA_DEFAULT_SIZE
	size_t SQ_ARENA_DEFAULT_SIZE = 0;
	if ((ctx->arena = arena_init(SQ_ARENA_DEFAULT_SIZE)) == nullptr) {
		return nullptr;
	}

	return ctx;
}

sq_server_context_t *sq_server_init_server_context(void) {
	sq_server_context_t *ctx = sq_server_init_context();
	if (!ctx) {
		return nullptr;
	}

	ctx->ctx_db.arena_overview_cnt = arena_init(SIZE_BLOCK_DEFAULT);
	ctx->ctx_db.is_arena_req = true;
	
	ctx->is_arena_req = true;
	return ctx;
}

bool sq_server_close_context(sq_server_context_t *ctx, bool require_force_destroy) {
	if (!ctx) {
		return false;
	}

	if (ctx->h_sock.is_running) {
		close(ctx->h_sock.fd);
		ctx->h_sock.is_running = false;

		if (ctx->is_arena_req) {
			status_t st = arena_destroy(ctx->arena, require_force_destroy);
			if (CND_ABORT == get_cnd(st)) {
				return false;
			}
		}
	}
	return true;
}

status_t sq_server_destory_server_context(sq_server_context_t *ctx, bool require_force_destroy) {
	if (!ctx) return asstatus(CAT_SERVER, CND_ABORT, CODE_VALUE);
		
	if (ctx->h_sock.is_running && ctx->h_sock.fd > -1) {
		close(ctx->h_sock.fd);
		ctx->h_sock.is_running = false;
		arena_set_contains_fd(ctx->arena, false);
	}

	if (ctx->arena && ctx->is_arena_req) {
		status_t ret = arena_destroy(ctx->arena, require_force_destroy);
		if (CND_ABORT == get_cnd(ret)) return ret;
	}

	sq_db_close(&ctx->ctx_db);
	sq_db_close_context(&ctx->ctx_db, require_force_destroy);
	return asstatus(CAT_SERVER, CND_SUCCESS, CODE_DESTROY);
}

void start_listening(sq_server_context_t *ctx_server) {
	if (ctx_server == nullptr) exit(EXIT_FAILURE);
	
	int server_fd;
	struct sockaddr_un addr;

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SQ_SOCKET_PATH, sizeof(addr.sun_path) - 1);
	unlink(SQ_SOCKET_PATH);
	
	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
		close(server_fd);
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 5) == -1) {
		close(server_fd);
		perror("listen");
		exit(EXIT_FAILURE);
	}

	ctx_server->h_sock.fd = server_fd;
	ctx_server->h_sock.is_running = true;
	arena_set_contains_fd(ctx_server->arena, true);
}

status_t sq_send_header(sq_server_context_t *ctx, int client_fd, sq_packet_header_t *header) {
	if (!header) return asstatus(CAT_SERVER, CND_ABORT, CODE_VALUE);
	(void)ctx;

	const char *ptr = (const char *)header;
	size_t total_sent = 0;
	size_t total_size = sizeof(sq_packet_header_t);

	while (total_sent < total_size) {
		ssize_t n = send(client_fd, ptr + total_sent, total_size - total_sent, 0);

		if (n > 0) {
			total_sent += n;
		} else if (n == 0) {
			fprintf(stderr, "Connection closed by client during send\n");
			close(client_fd);
			return asstatus(CAT_SERVER, CND_DEAD, CODE_CONNECTION);
		} else {
			if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				close(client_fd);
				return asstatus(CAT_RESPONSE, CND_RETRY, CODE_CONNECTION);
			}
			
			fprintf(stderr, "Send error: %s\n", strerror(errno));
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
			fprintf(stderr, "Connection refused by client\n");
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
void sq_handle_client_payload(sq_server_context_t *ctx_server, int client_fd) {
	sq_packet_header_t header;
	char *ptr = (char *)&header;
	size_t total_received = 0;
	size_t to_receive = sizeof(sq_packet_header_t);

	while (total_received < to_receive) {
		ssize_t n = recv(client_fd, ptr + total_received, to_receive - total_received, 0);
		if (n > 0) {
			total_received += n;
		} else if (n == 0) {
			fprintf(stderr, "Connection closed by client.\n");
			close(client_fd);
			return; 
		} else {
			if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK) return;
			close(client_fd);
			return;
		}
	}

	if (header.magic != SQ_MAGIC) return;
	
	if (header.payload_size > 0) {
		char *payload = arena_alloc(ctx_server->arena, sizeof(header.payload_size));
		if (!payload) return;
		ssize_t total_recv = 0;
		while (total_recv < (ssize_t)header.payload_size) {
			ssize_t r = recv(client_fd, payload + total_recv, header.payload_size - total_recv, 0);
			if (r <= 0) break;
			total_recv += r;
		}
		if (total_recv == (ssize_t)header.payload_size) {
			char *cmd_ptr = payload;
			size_t cmd_len = strlen(cmd_ptr);
			char *out_ptr = (cmd_len + 1 < header.payload_size) ? payload + cmd_len + 1 : NULL;
			// Changed structture packet header; sq_packet_header_t and sq_packet_body_t.
			// sq_db_save_backlog(ctx_server, &header.content, cmd_ptr, out_ptr);
			printf("[LOGGED] %s (Output: %zu bytes)\n", cmd_ptr, out_ptr ? strlen(out_ptr) : 0);
		}
		free(payload);
	}
}
