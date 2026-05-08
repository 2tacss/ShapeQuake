#include "engine/net.h"
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
}


/**
 * Handle data from a shell client and persist to SQLite.
 */
void sq_handle_client_payload(int client_fd) {
	sq_header_t header;
	char *ptr = (char *)&header;
	size_t total_received = 0;
	size_t to_receive = sizeof(sq_header_t);

	while (total_received < to_receive) {
		ssize_t n = recv(client_fd, ptr + total_received, to_receive - total_received, 0);
		if (n > 0) {
			total_received += n;
		} else if (n == 0) {
			fprintf(stderr, "Connection closed by client.\n");
			return; 
		} else {
			if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
			return;
		}
	}

	if (header.magic != SQ_MAGIC) return;

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
			sq_db_save_backlog(&header.context, cmd_ptr, out_ptr);
			
			printf("[LOGGED] %s (Output: %zu bytes)\n", cmd_ptr, out_ptr ? strlen(out_ptr) : 0);
		}
		sq_free(payload);
	}
}

