#ifndef SQ_CORE_ALLOCATOR_H
#define SQ_CORE_ALLOCATOR_H

#include <stddef.h>
#include "common.h"
#include "defines.h"



typedef struct sq_arena_block {
	unsigned short id;
	bool is_wiped; // data[] is or not if full zeroing by shred()
	struct sq_arena_block *next;
	struct sq_arena_block *prev;
	size_t offset;
	size_t capacity;
	alignas(16) char data[];
} sq_arena_block_t;

typedef struct {
	sq_arena_block_t *head;
	sq_arena_block_t *current;
	size_t block_size;
	bool contains_fd;
	bool contains_db_handle;
#ifdef DEBUG
	struct {
	const char *last_caller;
	size_t last_alloc_size;
	} stats;
#endif
} sq_arena_t;

/**
 * Sizes
 */
constexpr size_t SQ_NULL_LENGTH = 0;
constexpr size_t SQ_SIZE_BLOCK_DEFAULT = 1024;
constexpr size_t SQ_SIZE_BLOCK_META = offsetof(sq_arena_block_t, data);

/**
 * Return Flags
 */
constexpr sq_u16_t SQ_ARENA_SUCCESS = 0x0000;
constexpr sq_u16_t SQ_ARENA_ABORT_RESET = 0x0001;
constexpr sq_u16_t SQ_ARENA_FAILURE_RESOURCE_HELD = 0x0002;

/**
 * Argument Flags
 */
constexpr bool SQ_ARENA_REQUEST_RESET_OFFSET = true;
constexpr bool SQ_ARENA_FORCE_DESTROY = true;

/* Paging size */
static inline size_t sq_align(size_t size) {
	return (size + 15) & ~15;
}

/**
 * When failed exit safely or return NULL with writing message to error log.
 */
SQ_NODISCARD void *sq_malloc(size_t size);

/**
 * Re-Allocates.
 */
SQ_NODISCARD void *sq_realloc(void *ptr, size_t size);


/**
 * Make duplicates strings.
 */
SQ_NODISCARD char *sq_strdup(const char *s);

void sq_free(void *ptr);

/**
 * Arena Manage
 */
sq_arena_t *sq_arena_init(size_t block_size);
void *sq_arena_alloc_impl(sq_arena_t *arena, size_t size, const char *file, int line, const char *func);
void *sq_arena_alloc(sq_arena_t *arena, size_t size);
void *sq_arena_alloc_fd(sq_arena_t *arena, size_t size);
void *sq_arena_alloc_db_connection(sq_arena_t *arena, size_t size);
void *sq_arena_alloc_resources(sq_arena_t *arena, size_t size);
size_t sq_get_amount_capacity(sq_arena_block_t *head);
bool sq_arena_shred(sq_arena_t *arena, short id, bool request_reset_offset);
bool sq_block_shred(sq_arena_block_t *block, bool request_reset_offset);
sq_u16_t sq_arena_destroy(sq_arena_t *arena, bool force_destory);

#endif
