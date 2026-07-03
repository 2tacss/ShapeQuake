#include "hashmap.h"
#include "heap.h"

static heap_tracker_t heap_tracker = {0};
static hashmap_t static_hashmap = {0};
