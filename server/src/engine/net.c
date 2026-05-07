#include "engine/net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include "protocol.h"



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
