#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include <stddef.h>
#include "status.h"

/* Paging size */
static inline size_t align(size_t size) {
	return (size + 15) & ~15;
}

/**
 * Sizes
 */
constexpr size_t NULL_LENGTH = 0;
constexpr size_t SIZE_BLOCK_DEFAULT = 1024;
constexpr size_t SIZE_ARENA_DEFAULT = 1024;
constexpr size_t MAX_HEAPS = 20;

/**
 * Argument Flags
 */
constexpr bool ARENA_REQUEST_RESET_OFFSET = true;
constexpr bool ARENA_REQUEST_NO_RESET_OFFSET = false;
constexpr bool ARENA_FORCE_DESTROY = true;
constexpr bool ARENA_FORCE_NO_DESTROY = false;


static void *default_malloc(void *ptr, size_t size, void *ctx);
static void default_free(void *ptr,  size_t size, void *ctx);

typedef struct {

typedef struct arena_block {
	unsigned short id;
	bool is_wiped; // data[] is or not if full zeroing by shred()
	struct arena_block *next;
	struct arena_block *prev;
	size_t offset;
	size_t capacity;
	alignas(16) unsigned char data[];
} arena_block_t;

typedef struct {
	arena_block_t *head;
	arena_block_t *current;
	size_t block_size;
	bool contains_fd;
	bool contains_db_handle;
#ifdef DEBUG
	struct {
	const char *last_caller;
	size_t last_alloc_size;
	} stats;
#endif
} arena_t;

/********************
 * Arena Management *
 ********************/
arena_t *arena_init(size_t block_size);
void *arena_alloc_impl(arena_t *arena, size_t size, const char *file, int line, const char *func);
void *arena_alloc(arena_t *arena, size_t size);
status_t arena_set_contains_fd(arena_t *arena, bool require_set);
status_t arena_set_contains_db_connection(arena_t *arena, bool require_set);
status_t arena_set_contains_resources(arena_t *arena, bool require_set);
size_t get_amount_capacity(arena_block_t *head);
status_t arena_shred(arena_t *arena, unsigned short id, bool request_reset_offset);
status_t block_shred(arena_block_t *block, bool request_reset_offset);
status_t arena_destroy(arena_t *arena, bool force_destory);

#endif
