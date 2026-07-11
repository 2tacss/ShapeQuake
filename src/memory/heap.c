#include "memory/heap.h"
#include <stdlib.h>
#include <string.h>

void tracker_init(heap_tracker_t *tracker, size_t block_size) {
	memset(tracker, 0, sizeof(heap_tracker_t));
	__asm__ volatile("" : : : "memory");
	tracker->block_size = block_size;
	pthread_mutex_init(&tracker->mutex, nullptr);
}

int heap_alloc(heap_tracker_t *tracker) {
	if (!tracker) return -1;

	pthread_mutex_lock(&tracker->mutex);

	if (tracker->used_mask == 0xFFFFFFFFFFFFFFFFULL) {
		pthread_mutex_unlock(&tracker->mutex);
		return -1;
	}

	uint64_t free_mask = ~tracker->used_mask;
	int found_idx = __builtin_ctzll(free_mask);

	if (found_idx < 0 || found_idx >= MAX_HEAPS) {
		pthread_mutex_unlock(&tracker->mutex);
		return -1;
	}

	void *ptr = malloc(tracker->block_size);
	if (!ptr) {
		pthread_mutex_unlock(&tracker->mutex);
		return -1;
	}

	memset(ptr, 0, tracker->block_size);
	__asm__ volatile("" : : : "memory");
	
	tracker->slots[found_idx].ptr = ptr;
	tracker->used_mask |= (1ULL << found_idx);
	tracker->allocated += tracker->block_size;

	pthread_mutex_unlock(&tracker->mutex);
	return found_idx;
}

void heap_free(heap_tracker_t *tracker, int idx) {
	if (!tracker || idx < 0 || idx >= MAX_HEAPS) return;

	pthread_mutex_lock(&tracker->mutex);

	if (tracker->used_mask & (1ULL << idx)) {
		if (tracker->slots[idx].ptr) {
			memset(tracker->slots[idx].ptr, 0, tracker->block_size);
			free(tracker->slots[idx].ptr);
			tracker->slots[idx].ptr = nullptr;
		}
		tracker->used_mask &= ~(1ULL << idx);
		tracker->allocated -= tracker->block_size;
	}

	pthread_mutex_unlock(&tracker->mutex);
}

void heap_free_all(heap_tracker_t *tracker) {
	if (!tracker) return;

	pthread_mutex_lock(&tracker->mutex);

	uint64_t mask = tracker->used_mask;

	while (mask > 0) {
		int i = __builtin_ctzll(mask);

		if (tracker->slots[i].ptr) {
			memset(tracker->slots[i].ptr, 0, tracker->block_size);
			__asm__ volatile("" : : : "memory");
			free(tracker->slots[i].ptr);
			tracker->slots[i].ptr = nullptr;
		}

		mask &= (mask - 1);
	}

	tracker->used_mask = 0;
	tracker->allocated = 0;

	pthread_mutex_unlock(&tracker->mutex);
}

void *heap_get_ptr(heap_tracker_t *tracker, int idx) {
	if (!tracker || idx < 0 || idx >= MAX_HEAPS) return nullptr;
	return (tracker->used_mask & (1ULL << idx)) ? tracker->slots[idx].ptr : nullptr;
}
