#ifndef SQ_SERVER_ENGINE_NET_H_
#define SQ_SERVER_ENGINE_NET_H_

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
	arena_t *arena;
	bool is_arena_req;
} sq_server_context_t;

sq_server_context_t *sq_server_init_context(void);
sq_server_context_t *sq_server_init_server_context(void);
bool sq_server_close_context(sq_server_context_t *ctx, bool require_force_destroy);
status_t sq_server_destory_server_context(sq_server_context_t *ctx, bool require_force_destroy);
void start_listening(sq_server_context_t *ctx_server);
void sq_handle_client_payload(sq_server_context_t *ctx_server, int client_fd);


#endif
