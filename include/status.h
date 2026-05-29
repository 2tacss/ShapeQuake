#ifndef SQ_STATUS_H_
#define SQ_STATUS_H_

#include <defines.h>
#include <stddef.h>

typedef enum : sq_u16_t {
	CAT_NONE = 0x1,
	CAT_SQ = 0xA,
	CAT_ARENA = 0xB,
	CAT_RESPONSE = 0xC,
	CAT_VALUE = 0xD,
} sq_return_cat;

typedef union {
	sq_u16_t raw;
	struct {
		sq_u16_t code : 12;
		sq_return_cat cat : 4;
	};
} sq_status_t;

#define STATUS_SQ(v) { .cat = CAT_SQ, .code = (v) }
#define STATUS_ARENA(v) { .cat = CAT_ARENA, .code = (v) }
#define STATUS_RESPONSE(v) { .cat = CAT_RESPONSE, .code = (v) }

constexpr sq_u16_t RETCODE_SUCCESS = 0x000;
constexpr sq_u16_t RETCODE_FAILURE = 0x001;
constexpr sq_u16_t RETCODE_NULL_VAL = 0x002;
constexpr sq_u16_t RETCODE_INVALID_PARAM = 0x003;
constexpr sq_u16_t RETCODE_INVALID_SIZE = 0x008;
constexpr sq_u16_t RETCODE_REQUIRE_RETRY = 0x004;
constexpr sq_u16_t RETCODE_INVALID_MAGIC = 0x005;
constexpr sq_u16_t RETCODE_CONNECTION_CLOSED = 0x006;
constexpr sq_u16_t RETCODE_CONNECTION_REFUSED = 0x007;
constexpr sq_u16_t RETCODE_SEND_FAILED = 0x008;
constexpr sq_u16_t RETCODE_ARENA_ABORT_RESET = 0x0010;
constexpr sq_u16_t RETCODE_ARENA_FAILURE_RESOURCE_HELD = 0x0020;
constexpr sq_u16_t RETCODE_ARENA_DONE_DESTROY = 0x0030;

sq_status_t sq_asstatus(sq_return_cat cat, sq_u16_t code);

static inline sq_u16_t sq_what_return_category(sq_status_t status) {
	switch (status.cat) {
		case CAT_SQ: return 0xA000;
		case CAT_ARENA: return 0xB000;
		case CAT_RESPONSE: return 0xC000;
		case CAT_VALUE: return 0xD000;
		default: return 0x1111;
	}
}

void sq_handle_status_exception(sq_status_t result);
sq_status_t sq_init_status(sq_return_cat cat);
sq_status_t sq_update_status_cat(sq_status_t status, sq_return_cat cat);
sq_status_t sq_update_status_retcode(sq_status_t status, sq_u16_t retcode);
sq_status_t sq_update_category(sq_u16_t cat);

#endif
