#ifndef SQ_NETWORK_NET_H
#define SQ_NETWORK_NET_H

/**
 * The server called `middleware` has a protocol defined in `project_root/include/protocol.h`
 */

#include <stdio.h>

struct sq_shell_s; 
typedef struct sq_shell_s sq_shell_t;


/**
 * Send execution metadata to the middleware.
 */
int get_server_connection(void);

/**
 * Send header to middleware server.
 * Header struct contains information for `backlog` using for Main node, Overview.
 * `sq_header_t` is defined in root/include/protocol.h.
 */
void sq_send_backlog(sq_shell_t *shell, const char *output, size_t output_len);


#endif
