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
 * Argument Flags
 */
constexpr bool SQ_ARENA_REQUEST_RESET_OFFSET = true;
constexpr bool SQ_ARENA_FORCE_DESTROY = true;

/* Paging size */
static inline size_t sq_align(size_t size) {
	return (size + 15) & ~15;
}

#endif // SHAPEQUAKE_COMMON_H
