#include "allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *sq_malloc(size_t size) {
	if (size == 0) return nullptr;
	void *ptr = malloc(size);
	if (!ptr) {
		perror("sq_malloc: Out of memory.");
		return nullptr;
	}
	return ptr;
}

void *sq_realloc(void *ptr, size_t size) {
	if (size == 0) {
		sq_free(ptr);
		return nullptr;
	}
	void *n_ptr = realloc(ptr, size);
	if (!n_ptr) {
		perror("sq_realloc: Out of memory.");
		return nullptr;
	}
	return n_ptr;
}

char *sq_strdup(const char *s) {
	if (!s) return nullptr;
	size_t len = strlen(s) + 1;
	char *n_s = sq_malloc(len);
	if (n_s) {
		memcpy(n_s, s, len);
	}
	return n_s;
}

void sq_free(void *ptr) {
	if (ptr) {
		free(ptr);
	}
}

static sq_arena_block_t *sq_arena_new_block(size_t size) {
	size_t total_size = sizeof(sq_arena_block_t) + size;
	sq_arena_block_t *block = sq_malloc(total_size);
	if (!block) return nullptr;

	block->next = nullptr;
	block->offset = 0;
	block->capacity = size;
	return block;
}

sq_arena_t *sq_arena_init(size_t block_size) {
	sq_arena_t *arena = sq_malloc(sizeof(sq_arena_t));
	if (!arena) return nullptr;

	arena->block_size = sq_align(block_size);
	arena->head = sq_arena_new_block(arena->block_size);
	if (!arena->head) {
		sq_free(arena);
		return nullptr;
	}

	arena->current = arena->head;
#ifdef DEBUG
	memset(&arena->stats, 0, sizeof(arena->stats));
#endif
	return arena;
}

void *sq_arena_alloc_impl(sq_arena_t *arena, size_t size, const char *file, int line, const char *func) {
	if (!arena || size == 0) return nullptr;

	size_t aligned_size = sq_align(size);
	if (arena->current->offset + aligned_size > arena->current->capacity) {
		if (arena->current->next) {
			arena->current = arena->current->next;
			arena->current->offset = 0;
		} else {
			size_t next_size = (aligned_size > arena->block_size) ? aligned_size : arena->block_size;
			sq_arena_block_t *new_block = sq_arena_new_block(next_size);
			if (!new_block) return nullptr;
			arena->current->next = new_block;
			arena->current = new_block;
		}
	}

	void *ptr = &arena->current->data[arena->current->offset];
	arena->current->offset += aligned_size;

#ifdef DEBUG
	arena->stats.last_caller = func;
	arena->stats.last_alloc_size = size;
	(void)file;
	(void)line;
#endif

	return ptr;
}

void sq_arena_reset(sq_arena_t *arena) {
	if (!arena) return;
	sq_arena_block_t *it = arena->head;
	while (it) {
		it->offset = 0;
		it = it->next;
	}
	arena->current = arena->head;
}

void sq_arena_destroy(sq_arena_t *arena) {
	if (!arena) return;
	sq_arena_block_t *block = arena->head;
	while (block) {
		sq_arena_block_t *next = block->next;
		sq_free(block);
		block = next;
	}
	sq_free(arena);
}
