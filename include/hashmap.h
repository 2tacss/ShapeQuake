#ifndef HASHMAP_H_
#define HASHMAP_H_

#include "allocator.h"

typedef struct hashmap_node_t hashmap_node_t;
typedef struct hashmap_t hashmap_t;
typedef unsigned long hash_t;

#define HASHMAP_MAX_SIZE 1024
#define MAX_KEY_LEN  64
#define MAX_VAL_LEN  64

struct hashmap_node_t {
	char key[MAX_KEY_LEN];
	char val[MAX_VAL_LEN];
};

struct hashmap_t {
	arena_t arena; // arena_alloc(sizeof(node_t) * HASHMAP_MAX_SIZE); vma_handler
};

hashmap_t hasmpap_path;
hashmap_t hashmap_variable;

[[nodiscard]] hashmap_t hashmap_init(void);
[[nodiscard]] hash_t hash_create(const char *str);
void hashmap_insert(const char *key, const char *val);
const char *hashmap_getval(const char *key);
const char *hashmap_getkey(const char *val);


#endif
