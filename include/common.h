#ifndef SQ_COMMON_H
#define SQ_COMMON_H

#include "defines.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * C23: Type Aliases
 */
constexpr sq_u16_t SQ_SUCCESS = 0x0000;
constexpr sq_u16_t SQ_NULL_PTR = 0x0001;

/**
 * C23: Attributes
 * [[nodiscard]]: Ensures that function return values are handled.
 * [[maybe_unused]]: Suppresses warnings for intentionally unused parameters.
 */
#define SQ_NODISCARD [[nodiscard]]
#define SQ_MAYBE_UNUSED [[maybe_unused]]
#define SQ_NO_RETURN [[noreturn]]

#endif // SHAPEQUAKE_COMMON_H
