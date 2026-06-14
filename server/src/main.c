#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <signal.h>

#include "engine/net.h"
#include "protocol.h"
#include "engine/storage.h"
#include "allocator.h"

#define MAX_EVENTS 16

static volatile sig_atomic_t keep_running = 1;

/**
 * Signal handler for graceful shutdown.
 */
static void handle_sigint(int sig) {
	(void)sig;
	keep_running = 0;
}

int main(void) {
	sq_server_context_t *ctx = sq_server_init_context();
	sq_server_close_context(ctx, true);
	free(ctx);
	exit(EXIT_SUCCESS);
	
	int epoll_fd;
	arena_t *server_arena;
	sq_server_context_t *ctx_server;
	struct epoll_event ev, events[MAX_EVENTS];

	server_arena = arena_init(SIZE_BLOCK_DEFAULT);
	if (server_arena == nullptr) {
		// TODO: pass
	}
	ctx_server = arena_alloc(server_arena, sizeof(sq_server_context_t));
	ctx_server->arena = server_arena;
	ev.data.fd = ctx_server->h_sock.fd;

	signal(SIGINT, handle_sigint);

	sq_db_init(&ctx_server->ctx_db, "commands.db");
	start_listening(ctx_server);

	epoll_fd = epoll_create1(0);
	ev.events = EPOLLIN;
	ev.data.fd = ctx_server->h_sock.fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ctx_server->h_sock.fd, &ev);

	printf("ShapeQuake Middleware Server online. Persistence: SQLite3\n");

	while (keep_running) {
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 500);
		for (int i = 0; i < nfds; i++) {
			if (events[i].data.fd == ctx_server->h_sock.fd) {
				int client_fd = accept(ctx_server->h_sock.fd, NULL, NULL);
				if (client_fd != -1) {
					ev.events = EPOLLIN;
					ev.data.fd = client_fd;
					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
				}
			} else {
				sq_handle_client_payload(ctx_server, ev.data.fd);
			}
		}
	}

	printf("\nShutting down now...\n");
	sq_db_close(&ctx_server->ctx_db);
	close(ctx_server->h_sock.fd);
	unlink(SQ_SOCKET_PATH);
	arena_destroy(server_arena, ARENA_REQUEST_RESET_OFFSET);
	printf("\nShutdown safely.\n");

	return 0;
}
