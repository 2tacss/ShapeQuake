#include "shell/parser.h"
#include "protocol.h"
#include "shell/tokenizer.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/**
 * Parses the command line and allocates an execution payload.
 * It detects ShapeQuake-specific prefixes like 'subnode-Iv'.
 */
sq_payload_exec_t* sq_parser_create_payload(const char *line, uint32_t *out_flags) {
	sq_token_list_t *tokens = sq_tokenize(line);
	if (!tokens || tokens->count == 0) return NULL;

	uint32_t flags = 0;
	char shape[32] = "default";
	int cmd_start = 0;

	/* Prefix detection logic */
	if (strcmp(tokens->tokens[0], "subnode-Iv") == 0) {
		flags |= 0x01; /* SQ_FLAG_INTERACTIVE */
		strncpy(shape, "hexagon", 32);
		cmd_start = 1;
	} else if (strcmp(tokens->tokens[0], "subnode") == 0) {
		cmd_start = 1;
		/* Potential for --shape or --lock parsing here */
	}

	/* Reconstruct the command string from remaining tokens */
	/* (Simplified for PoC: joining with spaces) */
	char cmd_buf[1024] = {0};
	for (size_t i = cmd_start; i < tokens->count; i++) {
		strcat(cmd_buf, tokens->tokens[i]);
		if (i < tokens->count - 1) strcat(cmd_buf, " ");
	}

	size_t cmd_len = strlen(cmd_buf) + 1;
	sq_payload_exec_t *payload = malloc(sizeof(sq_payload_exec_t) + cmd_len);
	
	if (payload) {
		payload->flags = flags;
		strncpy(payload->shape_hint, shape, 32);
		payload->cmd_len = cmd_len;
		memcpy(payload->command_line, cmd_buf, cmd_len);
		*out_flags = flags;
	}

	/* Clean up tokens */
	// sq_tokenizer_free(tokens); 

	return payload;
}
