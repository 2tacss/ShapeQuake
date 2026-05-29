#include "common.h"
#include "error/sq_error.h"
#include "protocol.h"
#include "defines.h"
#include "validate.h"

sq_u16_t validate_packect_header(sq_packet_header_t *h) {
	if (!h) {
		SqErr.fatal("Invalid Param");
	} else if (h->magic != SQ_MAGIC) {
		// FIX: require code connection handling
		return (SQ_INVALID_MAGIC | SQ_RETURN_CAT_RESPONSE);
	} else if (h->payload_size < 1) {
		return(SQ_INVALID_SIZE | SQ_RETURN_CAT_RESPONSE);
	} else {
		return (SQ_SUCCESS | SQ_RETURN_CAT_RESPONSE);
	}
}

