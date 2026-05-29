#ifndef SQ_COMMON_H
#define SQ_COMMON_H

#include "defines.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * C23: Attributes
 * [[nodiscard]]: Ensures that function return values are handled.
 * [[maybe_unused]]: Suppresses warnings for intentionally unused parameters.
 */
#define SQ_NODISCARD [[nodiscard]]
#define SQ_MAYBE_UNUSED [[maybe_unused]]
#define SQ_NO_RETURN [[noreturn]]

/**
 * Sizes
 */
constexpr size_t SQ_NULL_LENGTH = 0;
constexpr size_t SQ_SIZE_BLOCK_DEFAULT = 1024;


/**
 * Return Flags (Type Safety)
 */
typedef union {
	sq_u16_t raw;
	struct {
		sq_u16_t code : 12;
		sq_u16_t cat : 4;
	};
} sq_status_t;

enum : sq_u16_t {
	CAT_NONE = 0x1,
	CAT_SQ = 0xA,
	CAT_ARENA = 0xB,
	CAT_RESPONSE = 0xC,
};

#define SQ_MAKE_STATUS(c, v) ((sq_status_t){ .cat = (c), .code = (v) })

constexpr sq_status_t SQ_SUCCESS = SQ_MAKE_STATUS(CAT_SQ, 0x000);
constexpr sq_status_t SQ_FAILURE = SQ_MAKE_STATUS(CAT_SQ, 0x001);
constexpr sq_status_t SQ_NULL_VAL = SQ_MAKE_STATUS(CAT_SQ, 0x002);
constexpr sq_status_t SQ_INVALID_PARAM = SQ_MAKE_STATUS(CAT_SQ, 0x003);
constexpr sq_status_t SQ_INVALID_SIZE = SQ_MAKE_STATUS(CAT_SQ, 0x008);
constexpr sq_status_t SQ_REQUIRE_RETRY = SQ_MAKE_STATUS(CAT_SQ, 0x004);
constexpr sq_status_t SQ_INVALID_MAGIC = SQ_MAKE_STATUS(CAT_SQ, 0x005);
constexpr sq_status_t SQ_CONNECTION_CLOSED = SQ_MAKE_STATUS(CAT_SQ, 0x006);
constexpr sq_status_t SQ_SEND_FAILED = SQ_MAKE_STATUS(CAT_SQ, 0x007);

constexpr sq_status_t SQ_ARENA_SUCCESS = SQ_MAKE_STATUS(CAT_ARENA, 0x000);
constexpr sq_status_t SQ_ARENA_ABORT_RESET = SQ_MAKE_STATUS(CAT_ARENA, 0x010);
constexpr sq_status_t SQ_ARENA_FAILURE_RESOURCE_HELD = SQ_MAKE_STATUS(CAT_ARENA, 0x020);

static inline sq_u16_t sq_what_return_category(sq_status_t status) {
	switch (status.cat) {
		case CAT_SQ: return 0xA000;
		case CAT_ARENA: return 0xB000;
		case CAT_RESPONSE: return 0xC000;
		default: return 0x1111;
	}
}

/**
 * Argument Flags
 */
constexpr bool SQ_ARENA_REQUEST_RESET_OFFSET = true;
constexpr bool SQ_ARENA_FORCE_DESTROY = true;

/* Paging size */
static inline size_t sq_align(size_t size) {
	return (size + 15) & ~15;
}

#endif // SHAPEQUAKE_COMMON_H
