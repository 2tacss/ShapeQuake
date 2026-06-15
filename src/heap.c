#include "heap.h"
#include "status.h"
#include <unistd.h>
#include <stdlib.h>

/*******************
 * Heap Management *
 *******************/
[[nodiscard]]
status_t tracking_health(heap_tracker_t *tracker) {
	if (!tracker) {
		return asstatus(CAT_HEAP_TRACKER, CND_ABORT, CODE_PARAM);
	}
	
    if ((tracker->active == 0 && tracker->allocated) || (tracker->active > 0 && !tracker->allocated)) {
		return asstatus(CAT_HEAP_TRACKER, CND_FATAL, CODE_CALCULATION);
	} else {
		return asstatus(CAT_HEAP_TRACKER, CND_SUCCESS, CODE_CALCULATION);
	}
}

void tracker_init(heap_tracker_t *tracker) {
    if (!tracker) return;
    for (size_t i = 0; i < MAX_HEAPS; i++) {
        tracker->ptr[i] = nullptr;
    }
}

void *heap_alloc(heap_tracker_t *tracker, size_t required_size) {
    if (!tracker || required_size < 1) return nullptr;
    if (tracker->active >= MAX_HEAPS) return nullptr;

    size_t req_aligned = align_heap(required_size);
    size_t total_aligned_size = req_aligned + 16;
    void *ptr = malloc(total_aligned_size);
    if (!ptr) return nullptr;

    volatile unsigned char *pointer = (volatile unsigned char *)ptr;
    for (size_t i = 0; i < total_aligned_size; i++) pointer[i] = 0;
    *(size_t *)ptr = req_aligned;

    void *user_ptr = (unsigned char *)ptr + 16;
    tracker->ptr[tracker->active] = user_ptr;
    tracker->active++;
    tracker->allocated += req_aligned;

    return user_ptr;
}

void heap_free_all(heap_tracker_t *tracker) {
    if (!tracker) return;
    
    for (size_t i = 0; i < tracker->active; i++) {
        if (tracker->ptr[i]) {
            void *raw_ptr = (unsigned char *)tracker->ptr[i] - 16;
            size_t size = *(size_t *)raw_ptr;
            
            volatile unsigned char *p = (volatile unsigned char *)raw_ptr;
            for (size_t j = 0; j < size + 16; j++) p[j] = 0;
            
            free(raw_ptr);
            tracker->ptr[i] = nullptr;
        }
    }
    tracker->active = 0;
    tracker->allocated = 0;
}

status_t heap_free(heap_tracker_t *tracker, void *ptr) {
    if (!ptr) return asstatus(CAT_HEAP, CND_SUCCESS, CODE_FREE);

    if (!tracker) {
        return asstatus(CAT_HEAP, CND_ABORT, CODE_PARAM);
    }

    void *raw_ptr = (unsigned char *)ptr - 16;
    size_t req_aligned = *(size_t *)raw_ptr;
    size_t total_aligned_size = req_aligned + 16;

    if (tracker->active == 0 || tracker->allocated < req_aligned) {
        return asstatus(CAT_HEAP, CND_ABORT, CODE_CALCULATION);
    }

    bool found = false;
    for (size_t i = 0; i < MAX_HEAPS; i++) {
        if (tracker->ptr[i] == ptr) {
            tracker->ptr[i] = nullptr;
            found = true;
            break;
        }
    }

    if (!found) {
        return asstatus(CAT_HEAP, CND_ABORT, CODE_NOT_FOUND);
    }

    volatile unsigned char *pointer = (volatile unsigned char *)raw_ptr;
    for (size_t i = 0; i < total_aligned_size; i++) {
        pointer[i] = 0;
    }

    free(raw_ptr);
    
    tracker->active--;
    tracker->allocated -= req_aligned;

    return asstatus(CAT_HEAP, CND_SUCCESS, CODE_FREE);
}

status_t rest_of_heap(heap_tracker_t *tracker) {
	if (!tracker) {
		return asstatus(CAT_HEAP_TRACKER, CND_ABORT, CODE_PARAM);
	}

	if (tracker->active > 0 && tracker->allocated) {
		return asstatus(CAT_HEAP, CND_INFO, CODE_EXIST);
	} else {
		return asstatus(CAT_HEAP, CND_INFO, CODE_NO_EXIST);
	}
}

