#ifndef SQ_PARSER_H_
#define SQ_PARSER_H_

#include "protocol.h"

sq_payload_exec_t* parser_create_payload(const char *line, uint32_t *out_flags);

#endif
