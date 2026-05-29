#ifndef SQ_STATUS_H_
#define SQ_STATUS_H_

#include <defines.h>
#include <stddef.h>

typedef union {
	stat_raw raw;
} sq_status_t;

/* ==========================================================================
 * CATEGORY: bit range from 48 to 63. (max 65535)
 * ========================================================================== */
constexpr stat_raw CAT_NONE					= 0x0001ULL << 48;
constexpr stat_raw CAT_SHELL				= 0x0010ULL << 48;
constexpr stat_raw CAT_TARMINAL				= 0x0020ULL << 48;
constexpr stat_raw CAT_NODE_MAIN			= 0x0030ULL << 48;
constexpr stat_raw CAT_NODE_MAIN_OVERVIEW	= 0x0031ULL << 48;
constexpr stat_raw CAT_NODE_SUB				= 0x0040ULL << 48;
constexpr stat_raw CAT_SHAPE				= 0x0050ULL << 48;
constexpr stat_raw CAT_PROG_LINE			= 0x0060ULL << 48;
constexpr stat_raw CAT_COLOR				= 0x0070ULL << 48;
constexpr stat_raw CAT_ARENA				= 0x00A0ULL << 48;
constexpr stat_raw CAT_RESPONSE				= 0x00B0ULL << 48;
constexpr stat_raw CAT_VALUE				= 0x00C0ULL << 48;
constexpr stat_raw CAT_SERVER				= 0x00D0ULL << 48;
constexpr stat_raw CAT_CLIENT				= 0x00E0ULL << 48;
constexpr stat_raw CAT_MAINFRAME			= 0x00F0ULL << 48;

/* ==========================================================================
 * CONDITION bit range from 32 to 47 (max 16)
 * ========================================================================== */
constexpr stat_raw CND_SUCCESS				= (1ULL << 0)  << 32;
constexpr stat_raw CND_FAILURE				= (1ULL << 1)  << 32;
constexpr stat_raw CND_INFO					= (1ULL << 2)  << 32;
constexpr stat_raw CND_DEBUG				= (1ULL << 3)  << 32;
constexpr stat_raw CND_WARN					= (1ULL << 4)  << 32;
constexpr stat_raw CND_FATAL				= (1ULL << 5)  << 32;
constexpr stat_raw CND_REQUIRE				= (1ULL << 6)  << 32;
constexpr stat_raw CND_REFUSE				= (1ULL << 7)  << 32;
constexpr stat_raw CND_DENIED				= (1ULL << 8)  << 32;
constexpr stat_raw CND_NULL					= (1ULL << 9)  << 32;
constexpr stat_raw CND_INVALID				= (1ULL << 10) << 32;
constexpr stat_raw CND_INTERRUPTION			= (1ULL << 11) << 32;
constexpr stat_raw CND_ABORT				= (1ULL << 12) << 32;
constexpr stat_raw CND_WAIT					= (1ULL << 13) << 32;
constexpr stat_raw CND_DEAD					= (1ULL << 14) << 32;
constexpr stat_raw CND_RETRY				= (1ULL << 15) << 32;

/* ==========================================================================
 * CODE: bottom before and after bit range 48 to 64 (max 65535).
 * ========================================================================== */
constexpr stat_raw CODE_PARAM				= 0x0001ULL << 16;
constexpr stat_raw CODE_SIZE				= 0x0002ULL << 16;
constexpr stat_raw CODE_MAGIC				= 0x0003ULL << 16;
constexpr stat_raw CODE_CONNECTION			= 0x0004ULL << 16;
constexpr stat_raw CODE_FILE				= 0x0005ULL << 16;
constexpr stat_raw CODE_DESC				= 0x0006ULL << 16;
constexpr stat_raw CODE_INTERFACE			= 0x0007ULL << 16;
constexpr stat_raw CODE_VALUE				= 0x0008ULL << 16;
constexpr stat_raw CODE_SEND				= 0x0009ULL << 16;
constexpr stat_raw CODE_RECV				= 0x000AULL << 16;
constexpr stat_raw CODE_OPEN				= 0x000BULL << 16;
constexpr stat_raw CODE_CLOSE				= 0x000CULL << 16;
constexpr stat_raw CODE_READ				= 0x000DULL << 16;
constexpr stat_raw CODE_WRITE				= 0x000EULL << 16;
constexpr stat_raw CODE_CONTEXT				= 0x000FULL << 16;

constexpr stat_raw CODE_ARENA_ABORT_RESET = 0x00000010;
constexpr stat_raw CODE_ARENA_FAILURE_RESOURCE_HELD = 0x00000020;
constexpr stat_raw CODE_ARENA_DONE_DESTROY = 0x00000030;

sq_status_t sq_asstatus(stat_raw cat, stat_raw code);

void sq_handle_status_exception(sq_status_t result);
sq_status_t sq_init_status(stat_raw cat);
sq_status_t sq_update_status_cat(sq_status_t status, stat_raw cat);
sq_status_t sq_update_status_retcode(sq_status_t status, stat_raw retcode);
sq_status_t sq_update_category(stat_raw cat);

#endif
