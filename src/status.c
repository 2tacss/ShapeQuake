#include "error/sq_error.h"
#include "defines.h"

sq_status_t sq_asstatus(sq_return_cat cat, sq_u16_t code) {
	return (sq_status_t){ .cat = cat, .code = code };
}

sq_status_t sq_init_status(sq_return_cat cat) {
	sq_status_t status;
	status.cat = cat;
	return status;
}

sq_status_t sq_update_status_cat(sq_status_t status, sq_return_cat cat) {
	status.cat = cat;
	return status;
}

sq_status_t sq_update_status_retcode(sq_status_t status, sq_u16_t retcode) {
	status.code = retcode;
	return status;
}

void sq_handle_status_exception(sq_status_t result) {
	switch (result.cat) {
		case CAT_SQ:
			if (result.code == RETCODE_INVALID_SIZE) {
			}
			break;
		case CAT_ARENA:
			break;
		case CAT_RESPONSE:
			break;
		default:
			break;
	}
}
