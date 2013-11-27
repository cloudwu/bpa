#ifndef bump_pointer_allocator_h
#define bump_pointer_allocator_h

#include <stdint.h>
#include <stdlib.h>

typedef uint32_t id;
struct bpa_heap;

struct bpa_heap * bpa_init(void *buffer, size_t sz);

id bpa_create(struct bpa_heap *, size_t sz);
void * bpa_grab(struct bpa_heap *, id);
void bpa_release(struct bpa_heap *, id);

void * bpa_get_ptr(struct bpa_heap *, id);
id bpa_get_id(struct bpa_heap *, void *);

// 4K bytes per step
// step 0 for full collect
void bpa_collect(struct bpa_heap *, int step);

// for debug
void bpa_dump(struct bpa_heap *);

#endif
