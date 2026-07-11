#ifndef HEAP_H_
#define HEAP_H_

#include <unistd.h>
#include <pthread.h>
#include "status.h"

/* Paging size */
static inline size_t align_heap(size_t size) {
	return (size + 15) & ~15;
}

constexpr size_t MAX_HEAPS = 20;

typedef struct {
	pthread_mutex_t mutex;
	size_t active;
	size_t allocated;
	unsigned char *ptr[MAX_HEAPS];
} heap_tracker_t;

/*******************
 * Heap Management *
 *******************/
void tracker_init(heap_tracker_t *tracker);
void tracker_destroy(heap_tracker_t *tracker);
[[nodiscard]] void *heap_alloc(heap_tracker_t *tracker, size_t size);
status_t heap_free(heap_tracker_t *tracker, void *ptr);
void heap_free_all(heap_tracker_t *tracker);
status_t tracking_health(heap_tracker_t *tracker);
status_t rest_of_heap(heap_tracker_t *tracker);

#endif
