#ifndef _VMA_H
#define _VMA_H

#include "allocator.h"
#include <stddef.h>
#include <stdbool.h>

#define MAX_SHM_NAME 32


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
vma_zone_t *vma_create(const char *name, size_t size);
vma_zone_t *vma_attach(const char *name, size_t size);
void *vma_alloc(vma_zone_t *zone, size_t size);
void vma_reset(vma_zone_t *zone);
void vma_unmap(vma_zone_t *zone);
void vma_free(vma_zone_t *zone);
void vma_destroy(vma_zone_t *zone);


/*************************
 * Allocator Integration *
 *************************/
mem_provider_t vma_get_provider(vma_zone_t *zone);

#endif

