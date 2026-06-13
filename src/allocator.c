#include "allocator.h"
#include "status.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/********************
 * Static Functions *
 ********************/
static arena_block_t *get_arena(arena_block_t *block_head, unsigned short id) {
	if (!block_head) return nullptr;

	arena_block_t *it = block_head;
	while (it) {
		if (it->id == id) {
			return it;
		}
		it = it->next;
	}
	return nullptr;
}

static arena_block_t *arena_new_block(size_t size) {
	size_t total_size = sizeof(arena_block_t) + size;
	size_t aligned = align(total_size);
	arena_block_t *block = malloc(aligned);
	if (!block) return nullptr;

	volatile unsigned char *ptr = (volatile unsigned char *)block;
	for (size_t i = 0; i < aligned; i++) {
		ptr[i] = 0;
	}

	block->id = 0;
	block->is_wiped = false;
	block->next = nullptr;
	block->prev = nullptr;
	block->offset = 0;
	block->capacity = size;
	return block;
}

/********************
 * Arena Management *
 ********************/
arena_t *arena_init(size_t block_size) {
	arena_t *arena = malloc(sizeof(arena_t));
	if (!arena) return nullptr;

	volatile unsigned char *ptr = (volatile unsigned char *)arena;
	for (size_t i = 0; i < sizeof(arena_t); i++) {
		ptr[i] = 0;
	}

	size_t bs = (block_size < 1) ? SIZE_BLOCK_DEFAULT : block_size;
	arena->block_size = align(bs);
	arena->head = arena_new_block(arena->block_size);
	if (!arena->head) {
		free(arena);
		return nullptr;
	}

	arena->current = arena->head;
#ifdef DEBUG
	memset(&arena->stats, 0, sizeof(arena->stats));
#endif
	return arena;
}

static inline void *arena_allocate_raw(arena_t *arena, size_t size) {
	size_t aligned_size = align(size);
	while (arena->current->offset + aligned_size > arena->current->capacity) {
		if (arena->current->next) {
			arena->current = arena->current->next;
			arena->current->offset = 0;
		} else {
			size_t next_size = (aligned_size > arena->block_size) ? aligned_size : arena->block_size;
			arena_block_t *new_block = arena_new_block(next_size);
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

void *arena_alloc_impl(arena_t *arena, size_t size, const char *file, int line, const char *func) {
	void *ptr = arena_allocate_raw(arena, size);
	if (!ptr) return nullptr;

	arena->stats.last_caller = func;
	arena->stats.last_alloc_size = size;
	(void)file;
	(void)line;

	return ptr;
}

void *arena_alloc(arena_t *arena, size_t size) {
	return arena_allocate_raw(arena, size);
}

status_t arena_set_contains_fd(arena_t *arena, bool require_set) {
	if (!arena) {
		return asstatus(CAT_VALUE, CND_FATAL, CODE_PARAM);
	}

	arena->contains_fd = require_set;

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_SET);
}

status_t arena_set_contains_db_connection(arena_t *arena, bool require_set) {
	if (!arena) {
		return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
	}

	arena->contains_db_handle = require_set;

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_SET);
}

status_t arena_set_contains_resources(arena_t *arena, bool require_set) {
	if (!arena) {
		return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
	}

	arena->contains_db_handle = require_set;
	arena->contains_fd = require_set;

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_SET);
}

void arena_reset(arena_t *arena) {
	if (!arena) return;
	arena_block_t *it = arena->head;
	while (it) {
		it->offset = 0;
		it = it->next;
	}
	arena->current = arena->head;
}

size_t get_amount_capacity(arena_block_t *head) {
	if (head == nullptr) return 0;

	unsigned int amount = 0;
	arena_block_t *it = head;

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
status_t arena_shred(arena_t *arena, unsigned short id, bool request_reset_offset) {
	if (!arena) return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);

	unsigned short id_target = (id < 0) ? arena->current->id : id;
	arena_block_t *block_target = get_arena(arena->head, id_target);

	if (block_target == nullptr) return asstatus(CAT_ARENA, CND_ABORT, CODE_ALLOC);

	volatile unsigned char *p = (volatile unsigned char *)block_target->data;
	for (size_t i = 0; i < block_target->capacity; i++) {
		p[i] = 0;
	}
	block_target->is_wiped = true;

	if (request_reset_offset) {
		block_target->offset = 0;
	}

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_CLEAR);
}

status_t block_shred(arena_block_t *block, bool request_reset_offset) {
	if (!block) return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);

	volatile unsigned char *p = (volatile unsigned char *)block->data;
	for (size_t i = 0; i < block->capacity; i++) {
		p[i] = 0;
	}
	block->is_wiped = true;

	if (request_reset_offset) {
		block->offset = 0;
	}

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_CLEAR);
}

status_t arena_destroy(arena_t *arena, bool force_destory) {
	if (!arena) return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
	if (!force_destory && ((arena->contains_db_handle || arena->contains_fd))) {
		return asstatus(CAT_ARENA, CND_FAILURE, CODE_ARENA_FAILURE_RESOURCE_HELD);
	}

	arena_block_t *block = arena->head;
	while (block) {
		arena_block_t *next = block->next;
		block_shred(block, ARENA_REQUEST_RESET_OFFSET);
		free(block);
		block = next;
	}
	free(arena);
	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_DESTROY);
}
