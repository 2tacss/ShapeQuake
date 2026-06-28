#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include <pthread.h>
#include <stddef.h>
#include "status.h"

/* Paging size */
static inline size_t align(size_t size) {
	return (size + 15) & ~15;
}


typedef int arena_id_t; 

/**
 * Sizes
 */
constexpr size_t NULL_LENGTH = 0;
constexpr size_t SIZE_BLOCK_DEFAULT = 1024;
constexpr size_t SIZE_ARENA_DEFAULT = 1024;

/**
 * Argument Flags
 */
constexpr bool ARENA_REQUEST_RESET_OFFSET = true;
constexpr bool ARENA_REQUEST_RESET_OFFSET_NO = false;
constexpr bool ARENA_FORCE_DESTROY = true;
constexpr bool ARENA_FORCE_DESTROY_NO = false;
constexpr arena_id_t ARENA_ID_CURRENT = -1;
constexpr arena_id_t ARENA_ID_ENTIRE  = -2;


static void *default_malloc(void *ptr, size_t size, void *ctx);
static void default_free(void *ptr,  size_t size, void *ctx);

typedef struct {
    void *(*alloc_raw)(void *ptr, size_t size, void *ctx);
    void  (*free_raw)(void *ptr, size_t size, void *ctx);
    void *ctx;
} mem_provider_t;

static mem_provider_t default_provider = {
	.alloc_raw = default_malloc,
	.free_raw = default_free
};

static mem_provider_t *cur_provider = &default_provider;

typedef struct arena_block {
	unsigned short id;
	bool is_wiped; // data[] is or not if full zeroing by shred()
	mem_provider_t provider;
	struct arena_block *next;
	struct arena_block *prev;
	size_t offset;
	size_t capacity;
	alignas(16) unsigned char data[];
} arena_block_t;

typedef struct {
	pthread_mutex_t mutex;
	arena_block_t *head;
	arena_block_t *current;
	size_t block_size;
	bool contains_fd;
	bool contains_db_handle;
	bool is_reset;
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
status_t arena_set_contains_fd(arena_t *arena, bool is_contain);
status_t arena_set_contains_db_connection(arena_t *arena, bool is_contain);
status_t arena_set_contains_resources(arena_t *arena, bool is_contain);
status_t arena_is_contains_resources(arena_t *arena);
size_t get_amount_capacity(arena_block_t *head);
void arena_reset(arena_t *arena);
status_t arena_shred(arena_t *arena, arena_id_t id, bool request_reset_offset);
status_t arena_destroy(arena_t *arena, bool force_destory);
char *arena_strdup(arena_t *arena, const char *src);

#endif
