#ifndef SQ_SERVER_NET_H_
#define SQ_SERVER_NET_H_

#include "protocol.h"
#include "engine/storage.h"
#include "allocator.h"
#include <bits/sockaddr.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

typedef struct server_context_t {
	sq_socket_handle_t h_sock;
	sq_db_context_t ctx_db;
	sq_arena_t *arena;
} sq_server_context_t;

void start_listening(sq_server_context_t *ctx_server);
void sq_handle_client_payload(sq_server_context_t *ctx_server, int client_fd);


#endif
