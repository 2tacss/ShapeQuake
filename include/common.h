#ifndef SQ_COMMON_H
#define SQ_COMMON_H

#include "defines.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * C23: Type Aliases
 */
typedef uint8_t byte;

/**
 * C23: Attributes
 * [[nodiscard]]: Ensures that function return values are handled.
 * [[maybe_unused]]: Suppresses warnings for intentionally unused parameters.
 */
#define SQ_NODISCARD [[nodiscard]]
#define SQ_MAYBE_UNUSED [[maybe_unused]]
#define SQ_NO_RETURN [[noreturn]]

#endif // SHAPEQUAKE_COMMON_H
