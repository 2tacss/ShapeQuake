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
 * Return Flags
 */
constexpr sq_u16_t SQ_RETURN_CAT_NON_CATEGORY = 0x1111;
constexpr sq_u16_t SQ_RETURN_CAT_SQ = 0xA000;
constexpr sq_u16_t SQ_RETURN_CAT_ARENA = 0xB000;
constexpr sq_u16_t SQ_RETURN_CAT_RESPONSE = 0xC000;
constexpr sq_u16_t SQ_SUCCESS = 0x0000;
constexpr sq_u16_t SQ_FAILURE = 0x0001;
constexpr sq_u16_t SQ_NULL_VAL = 0x0002;
constexpr sq_u16_t SQ_INVALID_PARAM = 0x0003;
constexpr sq_u16_t SQ_INVALID_SIZE = 0x0008;
constexpr sq_u16_t SQ_REQUIRE_RETRY = 0x0004;
constexpr sq_u16_t SQ_INVALID_MAGIC = 0x0005;
constexpr sq_u16_t SQ_CONNECTION_CLOSED = 0x0006;
constexpr sq_u16_t SQ_SEND_FAILED = 0x0007;

constexpr sq_u16_t SQ_ARENA_SUCCESS = 0x0000;
constexpr sq_u16_t SQ_ARENA_ABORT_RESET = 0x0010;
constexpr sq_u16_t SQ_ARENA_FAILURE_RESOURCE_HELD = 0x0020;

/**
 * Return Flag Extractor
 */
sq_u16_t what_return_category(sq_u16_t cat) {
	if (SQ_RETURN_CAT_SQ == (SQ_RETURN_CAT_SQ & cat)) {
		return SQ_RETURN_CAT_SQ;
	} else if (SQ_RETURN_CAT_ARENA == (SQ_RETURN_CAT_ARENA & cat)) {
		return SQ_RETURN_CAT_ARENA;
	} else {
		return SQ_RETURN_CAT_NON_CATEGORY;
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
