#pragma once
#include "heap.h"
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_HEAPS 64

typedef struct heap_slot_t heap_slot_t;
typedef struct heap_tracker_t heap_tracker_t;

struct heap_slot_t {
	void *ptr;
};

struct heap_tracker_t {
	heap_slot_t slots[MAX_HEAPS];
	uint64_t used_mask;
	size_t block_size;
	size_t allocated;
	pthread_mutex_t mutex;
};

void tracker_init(heap_tracker_t *tracker, size_t block_size);
int heap_alloc(heap_tracker_t *tracker);
void heap_free(heap_tracker_t *tracker, int idx);
void *heap_get_ptr(heap_tracker_t *tracker, int idx);
