#include "vma.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

static void *internal_vma_alloc(vma_zone_t *zone, size_t size) {
    size_t aligned = align_vma(size);

    if (zone->offset + aligned > zone->capacity) {
        return nullptr;
    }

	void *ptr = (void *)((uintptr_t)zone + zone->offset);
	zone->offset += aligned;
	return ptr;
}

static void vma_init(vma_zone_t *zone, int pshared) {
	if (pshared == 0) return;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    
    pthread_mutex_init(&zone->mutex, &attr);
    
    pthread_mutexattr_destroy(&attr);
}

vma_zone_t *vma_create(const char *name, size_t size, int pshared) {
	if (!name || size == 0) return nullptr;

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
		shm_unlink(name);
		return nullptr;
	}

	memset(base, 0, size);
	__asm__ volatile("" : : : "memory");

	vma_zone_t *zone = (vma_zone_t *)base;

	zone->fd = fd;
	zone->base_addr = base;
	zone->capacity = size;
	zone->offset = align_vma(sizeof(vma_zone_t));
	zone->is_master = true;
	vma_init(zone, pshared);
	strncpy(zone->name, name, sizeof(zone->name) - 1);
	zone->name[sizeof(zone->name) - 1] = '\0';

	return zone;
}

vma_zone_t *vma_attach(const char *name, size_t size) {
	if (!name || size == 0) return nullptr;

	int fd = shm_open(name, O_RDWR, 0);
	if (fd == -1) {
		perror("vma_attach: shm_open");
		return nullptr;
	}

	void *base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (base == MAP_FAILED) {
		perror("vma_attach: mmap failed");
		close(fd);
		return nullptr;
	}

	return (vma_zone_t *)base;
}

void *vma_alloc(vma_zone_t *zone, size_t size) {
    if (!zone) return nullptr;

    pthread_mutex_lock(&zone->mutex);
    void *ptr = internal_vma_alloc(zone, size);
    pthread_mutex_unlock(&zone->mutex);

    return ptr;
}

void vma_reset(vma_zone_t *zone) {
	if (!zone) return;

	size_t meta_reserved = align_vma(sizeof(vma_zone_t));
	if (zone->capacity > meta_reserved) {
		memset((unsigned char *)zone->base_addr + meta_reserved, 0, zone->capacity - meta_reserved);
		__asm__ volatile("" : : : "memory");
	}

	zone->offset = meta_reserved;
}

void vma_unmap(vma_zone_t *zone) {
	if (!zone) return;

	void *base = zone->base_addr;
	size_t cap = zone->capacity;
	int fd = zone->fd;

	if (base && base != MAP_FAILED) {
		munmap(base, cap);
	}

	if (fd != -1) {
		close(fd);
	}
}

void vma_free(vma_zone_t *zone) {
	if (!zone) return;
	vma_unmap(zone);
}

void vma_destroy(vma_zone_t *zone) {
	if (!zone) return;

	pthread_mutex_destroy(&zone->mutex);

	if (zone->is_master) {
		shm_unlink(zone->name);
	}

	vma_unmap(zone);
}

mem_provider_t vma_get_provider(vma_zone_t *zone) {
	return (mem_provider_t){
		.alloc_raw = vma_alloc_adapter,
		.free_raw = vma_free_adapter,
		.ctx = (void *)zone
	};
}
