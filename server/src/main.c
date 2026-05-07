#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <errno.h>
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

/**
 * Handle data from a shell client and persist to SQLite.
 */
static void handle_client_payload(int client_fd) {
	sq_header_t header;
	
	/* 1. Receive header */
	ssize_t n = recv(client_fd, &header, sizeof(sq_header_t), 0);
	if (n <= 0) {
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
		close(client_fd);
		return;
	}

	if (header.magic != SQ_MAGIC) return;

	/* 2. Receive payload (Command + Output) */
	if (header.payload_size > 0) {
		char *payload = sq_malloc(header.payload_size);
		if (!payload) return;

		ssize_t total_recv = 0;
		while (total_recv < (ssize_t)header.payload_size) {
			ssize_t r = recv(client_fd, payload + total_recv, header.payload_size - total_recv, 0);
			if (r <= 0) break;
			total_recv += r;
		}

		if (total_recv == (ssize_t)header.payload_size) {
			/* Separate pointers using null terminators */
			char *cmd_ptr = payload;
			size_t cmd_len = strlen(cmd_ptr);
			
			char *out_ptr = NULL;
			if (cmd_len + 1 < header.payload_size) {
				out_ptr = payload + cmd_len + 1;
			}

			/* Save to SQLite */
			db_save_command(&header.context, cmd_ptr, out_ptr);
			
			printf("[LOGGED] %s (Output: %zu bytes)\n", cmd_ptr, out_ptr ? strlen(out_ptr) : 0);
		}
		
		sq_free(payload);
	}
}

int main(void) {

	int server_fd, epoll_fd;
	struct sockaddr_un addr;
	struct epoll_event ev, events[MAX_EVENTS];

	signal(SIGINT, handle_sigint);

	if (db_init("commands.db") != 0) {
		fprintf(stderr, "Database initialization failed.\n");
		return 1;
	}

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket");
		return 1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SQ_SOCKET_PATH, sizeof(addr.sun_path) - 1);

	unlink(SQ_SOCKET_PATH);
	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
		perror("bind");
		return 1;
	}

	listen(server_fd, 5);

	epoll_fd = epoll_create1(0);
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

	printf("ShapeQuake Middleware Server online. Persistence: SQLite3\n");

	while (keep_running) {
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 500);
		for (int i = 0; i < nfds; i++) {
			if (events[i].data.fd == server_fd) {
				int client_fd = accept(server_fd, NULL, NULL);
				if (client_fd != -1) {
					ev.events = EPOLLIN;
					ev.data.fd = client_fd;
					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
				}
			} else {
				handle_client_payload(events[i].data.fd);
			}
		}
	}

	printf("\nShutting down now...\n");
	db_close();
	close(server_fd);
	unlink(SQ_SOCKET_PATH);
	printf("\nShutdown safely.\n");

	return 0;
}
