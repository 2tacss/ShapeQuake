#include "common.h"
#include "protocol.h"
#include "defines.h"
#include "validate.h"

sq_u16_t require_packect_header(sq_packet_header_t *h) {
	if (!h) {
		return (SQ_INVALID_PARAM | SQ_RETURN_CAT_RESPONSE);
	} else if (h->magic != SQ_MAGIC) {
		return (SQ_INVALID_MAGIC | SQ_RETURN_CAT_RESPONSE);
	} else if (h->payload_size < 1) {
		return(SQ_INVALID_SIZE | SQ_RETURN_CAT_RESPONSE);
	} else {
		return (SQ_SUCCESS | SQ_RETURN_CAT_RESPONSE);
	}
}

