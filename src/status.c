#include "status.h"
#include "defines.h"

sq_status_t sq_asstatus(sq_cat_t cat, sq_cnd_t condition, sq_code_t code) {
	return (sq_status_t){ .raw = cat | condition | code };
}

sq_status_t sq_init_status(sq_cat_t cat) {
	return (sq_status_t){ .raw = (stat_raw)cat & MASK_CAT };
}

sq_status_t sq_update_status_cat(sq_status_t st, sq_cat_t cat) {
	st.raw = (st.raw & ~MASK_CAT) | (stat_raw)cat;
	return st;
}

sq_status_t sq_update_status_cnd(sq_status_t st, sq_cnd_t cnd) {
	st.raw = (st.raw & ~MASK_CND) | (stat_raw)cnd;
	return st;
}

sq_status_t sq_update_status_code(sq_status_t st, sq_code_t code) {
	st.raw = (st.raw & ~MASK_CODE) | (stat_raw)code;
	return st;
}

sq_status_t sq_update_status_code_id(sq_status_t st, sq_code_t id) {
	st.raw = (st.raw & ~MASK_CODE_ID) | (stat_raw)id;
	return st;
}

sq_status_t sq_update_status_code_flg(sq_status_t st, sq_code_t flg) {
	st.raw = (st.raw & ~MASK_CODE_FLG) | (stat_raw)flg;
	return st;
}

sq_cat_t sq_get_cat(sq_status_t st) {
	return (sq_cat_t)(st.raw & MASK_CAT);
}

sq_cnd_t sq_get_cnd(sq_status_t st) {
	return (sq_cnd_t)(st.raw & MASK_CND);
}

sq_code_t sq_get_code(sq_status_t st) {
	return (sq_code_t)(st.raw & MASK_CODE);
}

sq_code_t sq_get_code_id(sq_status_t st) {
	return (sq_code_t)(st.raw & MASK_CODE_ID);
}

sq_code_t sq_get_code_flg(sq_status_t st) {
	return (sq_code_t)(st.raw & MASK_CODE_FLG);
}

void sq_handle_status_exception(sq_status_t st) {
	sq_cat_t cat  = (sq_cat_t)(st.raw & MASK_CAT);
	sq_cnd_t cnd  = (sq_cnd_t)(st.raw & MASK_CND);
	sq_code_t code = (sq_code_t)(st.raw & MASK_CODE);

	switch (cat) {
		case CAT_ARENA:
			if (cnd == CND_INVALID) {
			}
			break;

		case CAT_RESPONSE:
			break;

		case CAT_SERVER:
			break;

		default:
			break;
	}
}
