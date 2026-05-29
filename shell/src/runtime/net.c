#include "runtime/net.h"
#include "protocol.h"
#include "common.h"
#include "shell/shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sys/uio.h>

SQ_NODISCARD
int get_server_connection(void) {
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd != -1) {
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(struct sockaddr_un));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, SQ_SOCKET_PATH, sizeof(addr.sun_path) - 1);

		if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
			close(fd);
			fd = -1; 
		}
		return fd;
	}
	return -1;
}

/**
 * Send header to middleware server.
 * Header struct contains information for `backlog` using for Main node, Overview.
 * `sq_header_t` is defined in root/include/protocol.h.
 */
void sq_send_backlog(sq_shell_t *shell, const char *output, size_t output_len) {
	/* Check if the shell context and connection are valid */
	if (shell == nullptr || shell->net_middleware.fd == -1 || output == nullptr) {
		return;
	}

	/* Layout: [Command\0][Output\0] */
	size_t cmd_len = shell->line_len;
	size_t full_payload_size = (cmd_len + 1) + (output_len + 1);

	sq_header_t header = {
		.magic = SQ_MAGIC,
		.type = SQ_CAT_EXEC_RESULT | SQ_TYPE_OVERVIEW,
		.payload_size = (uint32_t)full_payload_size,
//		.context = { .timestamp = (uint64_t)time(nullptr) }
	};


	sq_body_t body = {0};
	body.content = {
			.timestamp = (uint64_t)time(nullptr),
		}
	};

	/* Set environment context info */
	if (getcwd(header.context.working_dir, sizeof(header.context.working_dir)) == nullptr) {
		strncpy(header.context.working_dir, "unknown", sizeof(header.context.working_dir) - 1);
	}
	strncpy(header.context.project_name, "ShapeQuake-Dev", sizeof(header.context.project_name) - 1);

	/* 1. Send protocol header */
	if (send(shell->net_middleware.fd, &header, sizeof(sq_header_t), 0) <= 0) {
		goto error_cleanup;
	}

	/* 2. Send Command string with null terminator */
	send(shell->net_middleware.fd, shell->line_buffer, cmd_len, 0);
	send(shell->net_middleware.fd, "\0", 1, 0);

	/* 3. Send Output data with null terminator */
	send(shell->net_middleware.fd, output, output_len, 0);
	send(shell->net_middleware.fd, "\0", 1, 0);

	return;

error_cleanup:
	/* Invalidate the fd on error to stop further attempts */
	close(shell->net_middleware.fd);
	shell->net_middleware.fd = -1;
}

