#include "allocator.h"
#include "status.h"
#include <pthread.h>
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
    size_t header_size = align(sizeof(arena_block_t));
    size_t data_size = align(size);
    size_t total_size = header_size + data_size;

    arena_block_t *block = malloc(total_size);
    if (!block) return nullptr;

    memset(block, 0, total_size);
    __asm__ volatile("" : : : "memory");

    block->id = 0;
    block->is_wiped = false;
    block->next = nullptr;
    block->prev = nullptr;
    block->offset = 0;
    block->capacity = data_size;
    return block;
}

static void internal_block_shred(arena_block_t *block, bool request_reset_offset) {
	memset(block->data, 0, block->capacity);
	__asm__ volatile("" : : : "memory");
	block->is_wiped = true;

	if (request_reset_offset) {
		block->offset = 0;
	}
}

static status_t internal_arena_shred(arena_t *arena, arena_id_t id, bool request_reset_offset) {
    if (id == ARENA_ID_ENTIRE) {
        for (arena_block_t *it = arena->head; it; it = it->next) {
            internal_block_shred(it, request_reset_offset);
        }
    } else if (id == ARENA_ID_CURRENT) {
        if (arena->current) internal_block_shred(arena->current, request_reset_offset);
    } else {
        arena_block_t *target = get_arena(arena->head, id);
        if (target) internal_block_shred(target, request_reset_offset);
        else return asstatus(CAT_ARENA, CND_ABORT, CODE_ALLOC);
    }
    return asstatus(CAT_ARENA, CND_SUCCESS, CODE_CLEAR);
}

/********************
 * Arena Management *
 ********************/
arena_t *arena_init(size_t block_size) {
	arena_t *arena = malloc(sizeof(arena_t));
	if (!arena) return nullptr;

	memset(arena, 0, sizeof(arena_t));
	__asm__ volatile("" : : : "memory");
	pthread_mutex_init(&arena->mutex, nullptr);
	
	size_t bs = (block_size < 1) ? SIZE_BLOCK_DEFAULT : block_size;
	arena->block_size = align(bs);
	arena->head = arena_new_block(arena->block_size);
	if (!arena->head) {
		// TODO: require shered
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
	pthread_mutex_lock(&arena->mutex);
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
	if (arena->is_reset) arena->is_reset = false;
	pthread_mutex_unlock(&arena->mutex);
	return ptr;
}

void *arena_alloc(arena_t *arena, size_t size) {
	return arena_allocate_raw(arena, size);
}

status_t arena_set_contains_fd(arena_t *arena, bool is_contain) {
	if (!arena) {
		return asstatus(CAT_VALUE, CND_FATAL, CODE_PARAM);
	}

	arena->contains_fd = is_contain;

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_SET);
}

status_t arena_set_contains_db_connection(arena_t *arena, bool is_contain) {
	if (!arena) {
		return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
	}

	pthread_mutex_lock(&arena->mutex);
	arena->contains_db_handle = is_contain;
	pthread_mutex_unlock(&arena->mutex);

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_SET);
}

status_t arena_set_contains_resources(arena_t *arena, bool is_contain) {
	if (!arena) {
		return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
	}

	pthread_mutex_lock(&arena->mutex);
	arena->contains_db_handle = is_contain;
	arena->contains_fd = is_contain;
	pthread_mutex_unlock(&arena->mutex);

	return asstatus(CAT_ARENA, CND_SUCCESS, CODE_SET);
}

status_t arena_is_contains_resources(arena_t *arena) {
	if (!arena) {
		return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
	}

	pthread_mutex_lock(&arena->mutex);
	if (arena->contains_db_handle || arena->contains_fd) {
		pthread_mutex_unlock(&arena->mutex);
		return asstatus(CAT_ARENA, CND_SUCCESS, CODE_CONTAINS);
	}
	pthread_mutex_unlock(&arena->mutex);
	return asstatus(CAT_ARENA, CND_FAILURE, CODE_CONTAINS);
}

void arena_reset(arena_t *arena) {
    if (!arena) return;

    pthread_mutex_lock(&arena->mutex);
    internal_arena_shred(arena, ARENA_ID_ENTIRE, ARENA_REQUEST_RESET_OFFSET); // internal版を呼ぶ！

    arena->current = arena->head;
    arena->is_reset = true;
    pthread_mutex_unlock(&arena->mutex);
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
status_t arena_shred(arena_t *arena, arena_id_t id, bool request_reset_offset) {
    if (!arena) return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
    pthread_mutex_lock(&arena->mutex);
    status_t st = internal_arena_shred(arena, id, request_reset_offset);
    pthread_mutex_unlock(&arena->mutex);
    return st;
}

status_t arena_destroy(arena_t *arena, bool force_destory) {
    if (!arena) return asstatus(CAT_ARENA, CND_FATAL, CODE_PARAM);
    
    pthread_mutex_lock(&arena->mutex);
    
    if (!force_destory && (arena->contains_db_handle || arena->contains_fd)) {
        pthread_mutex_unlock(&arena->mutex);
        return asstatus(CAT_ARENA, CND_FAILURE, CODE_ARENA_FAILURE_RESOURCE_HELD);
    }

    arena_block_t *block = arena->head;
    while (block) {
        arena_block_t *next = block->next;
        internal_block_shred(block, ARENA_REQUEST_RESET_OFFSET);
        free(block);
        block = next;
    }
    
    pthread_mutex_unlock(&arena->mutex);
    pthread_mutex_destroy(&arena->mutex);
    free(arena);
    return asstatus(CAT_ARENA, CND_SUCCESS, CODE_DESTROY);
}

char *arena_strdup(arena_t *arena, const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char *dest = (char *)arena_alloc(arena, len);
    if (dest) {
        memcpy(dest, src, len);
    }
    return dest;
}

/*
	06/27/2026
	Future implementation: arena_detach_block(arena_t *a, arena_id_t id);
	
	Data block `arena_block_t` in `arena_t` has data memory chain.
	This function implemented if need to detach field to manage alone arena_block_t.

	Or this,
	typedef struct arena_block {
		unsigned short id;
		bool is_wiped;
		bool is_persistent; /// ADD THIS
		mem_provider_t provider;
		struct arena_block *next;
		struct arena_block *prev;
		size_t offset;
		size_t capacity;
		alignas(16) unsigned char data[];
	} arena_block_t;

	If set is_persistent true,
	auto detach and remain stand alone memory field and a function returns the ptr.

	And like this finally in `destroy_arena()`:
	```
	while (block) {
		arena_block_t *next = block->next;
		if (!block->is_persistent) { // CHECK is_persistent
			block_shred(arena, block->id, ARENA_REQUEST_RESET_OFFSET);
			free(block);
		} else {
			// and detach here,
			// then return ptr.
		}
		block = next;
	}
	```
*/

