#ifndef STATUS_H_
#define STATUS_H_

#include <defines.h>
#include <stddef.h>



/**
 * Sizes
 */
constexpr size_t NULL_LENGTH = 0;
constexpr size_t SIZE_BLOCK_DEFAULT = 1024;

/**
 * Argument Flags
 */
constexpr bool ARENA_REQUEST_RESET_OFFSET = true;
constexpr bool ARENA_FORCE_DESTROY = true;


/* ==========================================================================
 * CATEGORY: bit range from 48 to 63 (max 65535)
 * ========================================================================== */
typedef enum : stat_raw {
	CAT_NONE					= 0x0001ULL << 48,
	CAT_SHELL					= 0x0010ULL << 48,
	CAT_TERMINAL				= 0x0020ULL << 48,
	CAT_NODE_MAIN				= 0x0030ULL << 48,
	CAT_NODE_MAIN_OVERVIEW		= 0x0031ULL << 48,
	CAT_NODE_SUB				= 0x0040ULL << 48,
	CAT_SHAPE					= 0x0050ULL << 48,
	CAT_PROG_LINE				= 0x0060ULL << 48,
	CAT_COLOR					= 0x0070ULL << 48,
	CAT_ARENA					= 0x00A0ULL << 48,
	CAT_RESPONSE				= 0x00B0ULL << 48,
	CAT_VALUE					= 0x00C0ULL << 48,
	CAT_SERVER					= 0x00D0ULL << 48,
	CAT_CLIENT					= 0x00E0ULL << 48,
	CAT_MAINFRAME				= 0x00F0ULL << 48,
	CAT_PROCESS					= 0x0A00ULL << 48,
	CAT_THREAD					= 0x0B00ULL << 48,
	CAT_HEAP					= 0x0F00ULL << 48,
	
	CAT_JOB						= 0x0C00ULL << 48,
	CAT_LOGGER					= 0x0D00ULL << 48,
	CAT_VIEW					= 0x0E00ULL << 48,
} cat_t;


/* ==========================================================================
 * CONDITION: bit range from 32 to 47 (max 16)
 * ========================================================================== */
typedef enum : stat_raw {
	CND_SUCCESS					= (1ULL << 0)  << 32,
	CND_FAILURE					= (1ULL << 1)  << 32,
	CND_INFO					= (1ULL << 2)  << 32,
	CND_DEBUG					= (1ULL << 3)  << 32,
	CND_WARN					= (1ULL << 4)  << 32,
	CND_FATAL					= (1ULL << 5)  << 32,
	CND_REQUIRE					= (1ULL << 6)  << 32,
	CND_REFUSE					= (1ULL << 7)  << 32,
	CND_DENIED					= (1ULL << 8)  << 32,
	CND_NULL					= (1ULL << 9)  << 32,
	CND_INVALID					= (1ULL << 10) << 32,
	CND_INTERRUPTION			= (1ULL << 11) << 32,
	CND_ABORT					= (1ULL << 12) << 32,
	CND_WAIT					= (1ULL << 13) << 32,
	CND_DEAD					= (1ULL << 14) << 32,
	CND_RETRY					= (1ULL << 15) << 32,
} cnd_t;


/* ==========================================================================
 * CODE: bit range from 0 to 31 (ID + FLG)
 * ========================================================================== */
typedef enum : stat_raw {
	/* CODE_ID: (from 16 to 31) */
	CODE_PARAM					= 0x0001ULL << 16,
	CODE_SIZE					= 0x0002ULL << 16,
	CODE_MAGIC					= 0x0003ULL << 16,
	CODE_CONNECTION				= 0x0004ULL << 16,
	CODE_FILE					= 0x0005ULL << 16,
	CODE_DESC					= 0x0006ULL << 16,
	CODE_SET					= 0x0007ULL << 16,
	CODE_VALUE					= 0x0008ULL << 16,
	CODE_SEND					= 0x0009ULL << 16,
	CODE_RECV					= 0x000AULL << 16,
	CODE_OPEN					= 0x000BULL << 16,
	CODE_CLOSE					= 0x000CULL << 16,
	CODE_READ					= 0x000DULL << 16,
	CODE_WRITE					= 0x000EULL << 16,
	CODE_ALLOC					= 0x000FULL << 16,
	CODE_CLEAR					= 0x0010ULL << 16,
	CODE_DESTROY				= 0x0020ULL << 16,
	CODE_FREE					= 0x0030ULL << 16,

	/* CODE_FLG: from 0 to 15 */
	CODE_CONTEXT				= 1ULL << 0,
} code_t;


/* ==========================================================================
 * BIT MASK
 * ========================================================================== */
constexpr stat_raw MASK_CAT					= 0xFFFFULL << 48; // 48〜63bit
constexpr stat_raw MASK_CND					= 0xFFFFULL << 32; // 32〜47bit
constexpr stat_raw MASK_CODE_ID				= 0xFFFFULL << 16; // 16〜31bit
constexpr stat_raw MASK_CODE_FLG				= 0xFFFFULL << 0;  //  0〜15bit
constexpr stat_raw MASK_CODE				= 0xFFFFFFFFULL;   // Entire CODE


/* ==========================================================================
 * SYSTEM STATUS CONTAINER
 * ========================================================================== */
typedef union {
	stat_raw raw;
} status_t;

/* ==========================================================================
 * BITS OVERFLOW / ALIGNMENT GUARD (C23 static_assert)
 * ========================================================================== */
static_assert((CAT_RESPONSE & 0x0000FFFFFFFFFFFFULL) == 0, "ERROR: CAT leaks into lower bits");
static_assert((CND_SUCCESS & 0xFFFF0000FFFFFFFFULL) == 0, "ERROR: CND overflows 47-32bit boundary");
static_assert((CND_INFO    & 0xFFFF0000FFFFFFFFULL) == 0, "ERROR: CND overflows 47-32bit boundary");
static_assert((CODE_WRITE   & 0xFFFFffff00000000ULL) == 0, "ERROR: CODE_ID leaks into upper bits");
static_assert((CODE_CONTEXT & 0xFFFFffff00000000ULL) == 0, "ERROR: CODE_FLG leaks into upper bits");

constexpr stat_raw CODE_ARENA_ABORT_RESET = 0x00000010;
constexpr stat_raw CODE_ARENA_FAILURE_RESOURCE_HELD = 0x00000020;
constexpr stat_raw CODE_ARENA_DONE_DESTROY = 0x00000030;

/* ==========================================================================
 * STATUS GATE INTERFACE
 * ========================================================================== */
status_t asstatus(cat_t cat, cnd_t condition, code_t code);
status_t init_status(cat_t cat);
status_t update_status_cat(status_t st, cat_t cat);
status_t update_status_cnd(status_t st, cnd_t cnd);
status_t update_status_code(status_t st, code_t code);
status_t update_status_code_id(status_t st, code_t id);
status_t update_status_code_flg(status_t st, code_t flg);
cat_t  get_cat(status_t st);
cnd_t  get_cnd(status_t st);
code_t get_code(status_t st);
code_t get_code_id(status_t st);
code_t get_code_flg(status_t st);
void handle_status_exception(status_t st);

#endif
