#ifndef SQ_CORE_ALLOCATOR_H
#define SQ_CORE_ALLOCATOR_H

#include <stddef.h>
#include "common.h"

typedef struct sq_arena_block {
 struct sq_arena_block *next;
 size_t offset;
 size_t capacity;
 alignas(16) char data[];
} sq_arena_block_t;

typedef struct {
	sq_arena_block_t *head;
	sq_arena_block_t *current;
	size_t block_size;
#ifdef DEBUG
	struct {
	const char *last_caller;
	size_t last_alloc_size;
	} stats;
#endif
} sq_arena_t;

/* Paging size  */
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
void sq_arena_destroy(sq_arena_t *arena);
void sq_arena_shred(sq_arena_t *arena);


#endif
