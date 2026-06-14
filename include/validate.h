#ifndef SQ_VALIDATE_H
#define SQ_VALIDATE_H

#include "common.h"
#include "protocol.h"
#include "status.h"
#include <stdint.h>
#include <stddef.h>

SQ_NODISCARD
status_t validate_packect_header(sq_packet_header_t *h);

#endif
