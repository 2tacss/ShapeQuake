#ifndef _VMA_H
#define _VMA_H

#include "allocator.h"
#include "heap.h"
#include <stddef.h>
#include <stdbool.h>



/* ========================================================================== *
 *  CHECK THIS OUT - VMA SHM LAYOUT                                           *
 * ========================================================================== *
 * Lower Addr ->   +-----------------------------------+
 *                 |  [Protected] VMA Meta Field       | (size: vma_zone_t)
 * Offset     ->   +-----------------------------------+
 *                 |  Array Element [0] (Logical ID 1) | (vma_alloc start)
 *                 |  Array Element [1] (Logical ID 2) |
 * Higher Addr ->  +-----------------------------------+
 *
 * 💡 No manual offset required in higher layers.
 * Logical IDs start from 1 (SHARED_ID_START_AT) and match slots naturally.
 * ========================================================================== */

#define MAX_SHM_NAME 32
#define SHARED_ID_START_AT 1

static inline size_t align_vma(size_t size) {
	return (size + 15) & ~15;
}


/****************
 * Data Types   *
 ****************/
typedef struct vma_zone_t vma_zone_t;

struct vma_zone_t {
	int fd;
	void *base_addr;
	size_t capacity;
	size_t offset;
	bool is_master;
	char name[MAX_SHM_NAME];
};

/*******************
 * Core VMA APIs   *
 *******************/
vma_zone_t *vma_create(heap_tracker_t *tracker, const char *name, size_t size);
vma_zone_t *vma_attach(heap_tracker_t *tracker, const char *name, size_t size);
void *vma_alloc(vma_zone_t *zone, size_t size);
void vma_reset(vma_zone_t *zone);
void vma_unmap(vma_zone_t *zone);
void vma_free(heap_tracker_t *tracker, vma_zone_t *zone);
void vma_destroy(heap_tracker_t *tracker, vma_zone_t *zone);


/*************************
 * Allocator Integration *
 *************************/
mem_provider_t vma_get_provider(vma_zone_t *zone);

#endif

