#include "status.h"
#include "defines.h"

status_t asstatus(cat_t cat, cnd_t condition, code_t code) {
	return (status_t){ .raw = cat | condition | code };
}

status_t init_status(cat_t cat) {
	return (status_t){ .raw = (stat_raw)cat & MASK_CAT };
}

status_t update_status_cat(status_t st, cat_t cat) {
	st.raw = (st.raw & ~MASK_CAT) | (stat_raw)cat;
	return st;
}

status_t update_status_cnd(status_t st, cnd_t cnd) {
	st.raw = (st.raw & ~MASK_CND) | (stat_raw)cnd;
	return st;
}

status_t update_status_code(status_t st, code_t code) {
	st.raw = (st.raw & ~MASK_CODE) | (stat_raw)code;
	return st;
}

status_t update_status_code_id(status_t st, code_t id) {
	st.raw = (st.raw & ~MASK_CODE_ID) | (stat_raw)id;
	return st;
}

status_t update_status_code_flg(status_t st, code_t flg) {
	st.raw = (st.raw & ~MASK_CODE_FLG) | (stat_raw)flg;
	return st;
}

cat_t get_cat(status_t st) {
	return (cat_t)(st.raw & MASK_CAT);
}

cnd_t get_cnd(status_t st) {
	return (cnd_t)(st.raw & MASK_CND);
}

code_t get_code(status_t st) {
	return (code_t)(st.raw & MASK_CODE);
}

code_t get_code_id(status_t st) {
	return (code_t)(st.raw & MASK_CODE_ID);
}

code_t get_code_flg(status_t st) {
	return (code_t)(st.raw & MASK_CODE_FLG);
}

void handle_status_exception(status_t st) {
	cat_t cat  = (cat_t)(st.raw & MASK_CAT);
	cnd_t cnd  = (cnd_t)(st.raw & MASK_CND);
	code_t code = (code_t)(st.raw & MASK_CODE);

	switch (cat) {
		case CAT_ARENA:
			if (cnd == CND_INVALID) {
				if (CODE_ARENA_ABORT_RESET == (code & MASK_CODE_ID)) {
					
				}
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
