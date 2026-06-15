#include "vma.h"
#include "allocator.h"
#include "heap.h"
#include "status.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static heap_tracker_t s_tracker = {0};

static void *vma_alloc_adapter(void *ptr, size_t size, void *ctx) {
	vma_zone_t *zone = (vma_zone_t *)ctx;
	if (!zone) return nullptr;

	if (!ptr) return nullptr;
	return vma_alloc(zone, size);
}

static void vma_free_adapter(void *ptr, size_t size, void *ctx) {
	(void)ptr;
	(void)size;
	(void)ctx;
}

vma_zone_t *vma_create(const char *name, size_t size) {
	if (!name || size == 0) return nullptr;

	status_t s = tracking_health(&s_tracker);
	if (CND_ABORT == get_cnd(s) || CND_FATAL == get_cnd(s)) return nullptr;

	int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		perror("vma_create");
		return nullptr;
	}

	if (ftruncate(fd, size) == -1) {
		perror("ftruncate");
		close(fd);
		shm_unlink(name);
		return nullptr;
	}

	void *base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (base == MAP_FAILED) {
		perror("mmap");
		close(fd);
		unlink(name);
		return nullptr;
	}

	vma_zone_t *zone = (vma_zone_t *)heap_alloc(&s_tracker, sizeof(vma_zone_t));
	if (!zone) {
		perror("heap_alloc");
		munmap(base, size);
		close(fd);
		unlink(name);
		return nullptr;
	}

	zone->fd = fd;
	zone->base_addr = base;
	zone->capacity = size;
	zone->offset = align_vma(sizeof(vma_zone_t));
	zone->is_master = true;
	strncpy(zone->name, name, sizeof(zone->name) - 1);
	zone->name[sizeof(zone->name) - 1] = '\0';

	volatile unsigned char *p = (volatile unsigned char *)base;
	for (size_t i = 0; i < size; i++) {
		p[i] = 0;
	}

	return zone;
}

vma_zone_t *vma_attach(const char *name, size_t size) {
	if (!name || size == 0) return nullptr;

	status_t s = tracking_health(&s_tracker);
	if (CND_ABORT == get_cnd(s) || CND_FATAL == get_cnd(s)) return nullptr;

	int fd = shm_open(name, O_RDWR, 0);
	if (fd == -1) {
		perror("vma_attach: shm_open");
		close(fd);
		return nullptr;
	}

	void *base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (base == MAP_FAILED) {
		perror("vma_attach: mmap failed");
		close(fd);
	}

	vma_zone_t *zone = (vma_zone_t *)heap_alloc(&s_tracker, sizeof(vma_zone_t));
	if (!zone) {
		perror("vma_attach: heap_alloc failed");
		munmap(base, size);
		close(fd);
		return nullptr;
	}

	zone->fd = fd;
	zone->base_addr = base;
	zone->capacity = size;
	zone->offset = align_vma(sizeof(vma_zone_t));
	zone->is_master = false;
	strncpy(zone->name, name, sizeof(zone->name) - 1);
	zone->name[sizeof(zone->name) - 1] = '\0';

	return zone;
}

void *vma_alloc(vma_zone_t *zone, size_t size) {
	if (!zone || size == 0) return 0;

	size_t aligned = align_vma(size);

	if (zone->offset + aligned > zone ->capacity) {
		fprintf(stderr, "vma_alloc: Out of memory in vma zone `%s`.\n", zone->name);
		return nullptr;
	}

	void *ptr = (void *)((uintptr_t)zone->base_addr + zone->offset);
	zone->offset += aligned;
	return ptr;
}

void vma_reset(vma_zone_t *zone) {
	if (!zone) return;

	volatile unsigned char *p = (volatile unsigned char *)zone->base_addr;
	for (size_t i = 0; i < zone->capacity; i++) {
		p[i] = 0;
	}

	zone->offset = align_vma(sizeof(vma_zone_t));
}

void vma_unmap(vma_zone_t *zone) {
	if (!zone) return;

	if (zone->base_addr && zone->base_addr != MAP_FAILED) {
		munmap(zone->base_addr, zone->capacity);
		zone->base_addr = MAP_FAILED;
	}

	if (zone->fd != -1) {
		close(zone->fd);
		zone->fd = -1;
	}
	return;
}

void vma_free(vma_zone_t *zone) {
	if (!zone) return;

	status_t s = tracking_health(&s_tracker);
	if (CND_ABORT == get_cnd(s) || CND_FATAL == get_cnd(s)) return;

	vma_unmap(zone);
	heap_free(&s_tracker, (void *)zone);
	return;
}

void vma_destroy(vma_zone_t *zone) {
	if (!zone) return;

	status_t s = tracking_health(&s_tracker);
	if (CND_ABORT == get_cnd(s) || CND_FATAL == get_cnd(s)) return;

	vma_unmap(zone);

	if (zone->is_master) {
		shm_unlink(zone->name);
	}

	heap_free(&s_tracker, (void *)zone);
}

mem_provider_t vma_get_provider(vma_zone_t *zone) {
	return (mem_provider_t){
		.alloc_raw = vma_alloc_adapter,
		.free_raw = vma_free_adapter,
		.ctx = (void *)zone
	};
}


