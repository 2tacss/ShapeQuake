#include "validate.h"
#include "protocol.h"
#include "status.h"
#include "error/sq_error.h"

status_t validate_packect_header(sq_packet_header_t *h) {
	if (!h) {
		SqErr.fatal("Invalid Param");
	} else if (h->magic != SQ_MAGIC) {
		// FIX: require code connection handling retry or close
		return asstatus(CAT_RESPONSE, CND_INVALID, CODE_MAGIC);
	} else if (h->payload_size < 1) {
		return asstatus(CAT_RESPONSE, CND_INVALID, CODE_SIZE);
	} else {
		return asstatus(CAT_RESPONSE, CND_SUCCESS, CODE_CONNECTION);
	}
}

