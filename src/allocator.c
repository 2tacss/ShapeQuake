#include "allocator.h"
#include <stddef.h>
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


/**
 * Arena Management
 */
static sq_arena_block_t *get_arena(sq_arena_block_t *block_head, unsigned short id) {
	if (!block_head) return nullptr;

	sq_arena_block_t *it = block_head;
	while (it) {
		if (it->id == id) {
			return it;
		}
		it = it->next;
	}
	return nullptr;
}

static sq_arena_block_t *sq_arena_new_block(size_t size) {
	size_t total_size = sizeof(sq_arena_block_t) + size;
	sq_arena_block_t *block = sq_malloc(total_size);
	if (!block) return nullptr;

	block->id = 0;
	block->is_wiped = false;
	block->next = nullptr;
	block->prev = nullptr;
	block->offset = 0;
	block->capacity = size;
	return block;
}

sq_arena_t *sq_arena_init(size_t block_size) {
	sq_arena_t *arena = sq_malloc(sizeof(sq_arena_t));
	if (!arena) return nullptr;

	size_t bs = (block_size < 1) ? SQ_SIZE_BLOCK_DEFAULT : block_size;
	arena->block_size = sq_align(bs);
	arena->head = sq_arena_new_block(arena->block_size);
	if (!arena->head) {
		sq_free(arena);
		exit(EXIT_FAILURE);	
	}

	arena->current = arena->head;
	arena->contains_fd = false;
	arena->contains_db_handle = false;
#ifdef DEBUG
	memset(&arena->stats, 0, sizeof(arena->stats));
#endif
	return arena;
}

static inline void *sq_arena_allocate_raw(sq_arena_t *arena, size_t size) {
	size_t aligned_size = sq_align(size);
	while (arena->current->offset + aligned_size > arena->current->capacity) {
		if (arena->current->next) {
			arena->current = arena->current->next;
			arena->current->offset = 0;
		} else {
			size_t next_size = (aligned_size > arena->block_size) ? aligned_size : arena->block_size;
			sq_arena_block_t *new_block = sq_arena_new_block(next_size);
			if (!new_block) {
				fprintf(stderr, "[FATAL]: Out of memory.\n");
				return nullptr;
			}
			new_block->id = arena->current->id + 1;
			new_block->prev = arena->current;
			arena->current->next = new_block;
			arena->current = new_block;
			break;
		}
	}

	void *ptr = &arena->current->data[arena->current->offset];
	arena->current->offset += aligned_size;
	return ptr;
}

void *sq_arena_alloc_impl(sq_arena_t *arena, size_t size, const char *file, int line, const char *func) {
	void *ptr = sq_arena_allocate_raw(arena, size);
	if (!ptr) return nullptr;

	arena->stats.last_caller = func;
	arena->stats.last_alloc_size = size;
	(void)file;
	(void)line;

	return ptr;
}

void *sq_arena_alloc(sq_arena_t *arena, size_t size) {
	return sq_arena_allocate_raw(arena, size);
}

void *sq_arena_alloc_fd(sq_arena_t *arena, size_t size) {
	sq_arena_t *ptr = (sq_arena_t *)sq_arena_allocate_raw(arena, size);
	ptr->contains_fd = true;
	ptr->contains_db_handle = false;
	return (void *)ptr;
}

void *sq_arena_alloc_db_connection(sq_arena_t *arena, size_t size) {
	sq_arena_t *ptr = (sq_arena_t *)sq_arena_allocate_raw(arena, size);
	ptr->contains_db_handle = true;
	ptr->contains_fd = false;
	return (void *)ptr;
}

void *sq_arena_alloc_resources(sq_arena_t *arena, size_t size) {
	sq_arena_t *ptr = (sq_arena_t *)sq_arena_allocate_raw(arena, size);
	ptr->contains_db_handle = true;
	ptr->contains_fd = true;
	return (void *)ptr;
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

size_t sq_get_amount_capacity(sq_arena_block_t *head) {
	if (head == nullptr) return 0;

	unsigned int amount = 0;
	sq_arena_block_t *it = head;

	while (it) {
		if (it->capacity < 1) {
			return SIZE_MAX;
		}
		amount += it->capacity;
		it = it->next;
	}
	return amount;
}

/**
* If id < 0, reference current id
*/
bool sq_arena_shred(sq_arena_t *arena, short id, bool request_reset_offset) {
	if (!arena) return false;

	unsigned short id_target = (id < 0) ? arena->current->id : (unsigned short)id;
	sq_arena_block_t *block_target = get_arena(arena->head, id_target);

	if (block_target == nullptr) return false;

	volatile char *p = (volatile char *)block_target->data;
	for (size_t i = 0; i < block_target->capacity; i++) {
		p[i] = 0;
	}
	block_target->is_wiped = true;

	if (request_reset_offset) {
		block_target->offset = 0;
	}

	return true;
}

bool sq_block_shred(sq_arena_block_t *block, bool request_reset_offset) {
	if (!block) return false;

	volatile char *p = (volatile char *)block->data;
	for (size_t i = 0; i < block->capacity; i++) {
		p[i] = 0;
	}
	block->is_wiped = true;

	if (request_reset_offset) {
		block->offset = 0;
	}

	return true;
}

sq_u16_t sq_arena_destroy(sq_arena_t *arena, bool force_destory) {
	if (!arena) return (SQ_RETURN_CAT_ARENA | SQ_NULL_VAL);
	if (!force_destory && ((arena->contains_db_handle || arena->contains_fd))) {
		return (SQ_RETURN_CAT_ARENA | SQ_ARENA_FAILURE_RESOURCE_HELD);
	}

	sq_arena_block_t *block = arena->head;
	while (block) {
		sq_arena_block_t *next = block->next;
		sq_block_shred(block, SQ_ARENA_REQUEST_RESET_OFFSET);
		sq_free(block);
		block = next;
	}
	sq_free(arena);
	return (SQ_RETURN_CAT_ARENA | SQ_SUCCESS);
}
