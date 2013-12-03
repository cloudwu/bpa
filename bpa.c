#include "bpa.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define ALIGN_SIZE 8
#define COLLECT_STEP_LENGTH 0x1000
#define HEAP_HEADER_SIZE ((sizeof(struct bpa_heap) + ALIGN_SIZE - 1) / ALIGN_SIZE)
#define HEAP_SLICE(H, off) (struct slice *)((char *)H + off * ALIGN_SIZE)
#define SLICE_ID(S) ((id *)((uintptr_t)(S) + S->sz * ALIGN_SIZE - sizeof(id)))
#define MAX_SLOT 0x10000

typedef uint32_t pointer_t;

struct bpa_heap {
	pointer_t sz;
	pointer_t offset;
	pointer_t collect;
	id cid;
	pointer_t slot[MAX_SLOT];
};

// the size of slice is at the end of slice, because sizeof(struct slice) align to 8 would be better.
struct slice {
	int ref;
	pointer_t sz;
};

struct bpa_heap * 
bpa_init(void *buffer, size_t sz) {
	struct bpa_heap *H = buffer;
	sz /= ALIGN_SIZE;
	if (sz != (pointer_t)sz)
		return NULL;
	if (sz <= HEAP_HEADER_SIZE) {
		return NULL;
	}
	H->sz = sz;
	H->offset = HEAP_HEADER_SIZE;
	H->collect = 0;
	H->cid = 0;

	return H;
}

static int
alloc_slot(struct bpa_heap *H) {
	int i;
	for (i=0;i<MAX_SLOT;i++) {
		id cid = ++ H->cid;
		if (cid == 0) {
			cid = ++ H->cid;
		}
		int slot = cid % MAX_SLOT;
		if (H->slot[slot] == 0) {
			return slot;
		}
	}
	return -1;
}

id 
bpa_create(struct bpa_heap *H, size_t sz) {
	pointer_t rsz = (sz + sizeof(struct slice) + sizeof(id) + ALIGN_SIZE - 1) / ALIGN_SIZE;
	if (H->offset + rsz > H->sz)
		return 0;
	int slot = alloc_slot(H);
	if (slot < 0) {
		return 0;
	}
	id cid = H->cid;
	H->slot[slot] = H->offset;
	struct slice * s = HEAP_SLICE(H, H->offset);
	H->offset += rsz;
	s->ref = 0;
	s->sz = rsz;
	*SLICE_ID(s) = cid;

	return cid;
}

void * 
bpa_grab(struct bpa_heap *H, id uid) {
	pointer_t off = H->slot[uid % MAX_SLOT];
	if (off == 0) {
		return NULL;
	}
	struct slice *s = HEAP_SLICE(H, off);
	if (*SLICE_ID(s) != uid) {
		return NULL;
	}
	++s->ref;
	return s+1;
}

void 
bpa_release(struct bpa_heap *H, id uid) {
	pointer_t off = H->slot[uid % MAX_SLOT];
	if (off == 0) {
		assert(0);
		return;
	}
	struct slice *s = HEAP_SLICE(H, off);
	if (*SLICE_ID(s) != uid) {
		assert(0);
		return;
	}
	assert(s->ref > 0);
	--s->ref;
}

void * 
bpa_get_ptr(struct bpa_heap *H, id uid) {
	pointer_t off = H->slot[uid % MAX_SLOT];
	if (off == 0) {
		return NULL;
	}
	struct slice *s = HEAP_SLICE(H, off);
	if (*SLICE_ID(s) != uid) {
		return NULL;
	}
	return s+1;
}

id 
bpa_get_id(struct bpa_heap *H, void *ptr) {
	assert((ptr > (void *)H) && ((char *)ptr < (char *)H + H->offset * ALIGN_SIZE));
	struct slice * s = (struct slice *)ptr - 1;
	return *SLICE_ID(s);
}

static void
collect(struct bpa_heap *H) {
	pointer_t p = H->collect;
	assert(p);
	int length = 0;
	pointer_t dst = H->collect;
	for (;;) {
		if (p >= H->offset) {
			H->offset = dst;
			H->collect = 0;
			return;
		}
		if (length > COLLECT_STEP_LENGTH) {
			H->collect = dst;
			if (p != dst) {
				struct slice *s = HEAP_SLICE(H, dst);
				s->ref = 0;
				s->sz = p - dst;
				*SLICE_ID(s) = 0;
			}
			break;
		}
		struct slice *s = HEAP_SLICE(H, p);
		pointer_t sz = s->sz;
		id uid = *SLICE_ID(s);
		length += sizeof(struct slice) ;
		if (s->ref == 0) {
			if (uid) {
				H->slot[uid % MAX_SLOT] = 0;
			}
		} else {
			if (dst != p) {
				H->slot[uid % MAX_SLOT] = dst;
				memmove(HEAP_SLICE(H, dst), s, sz * ALIGN_SIZE);
				length += sz * ALIGN_SIZE;
			}
			dst += sz;
		}
		p += sz;
	}
}

void
bpa_collect(struct bpa_heap * H, int step) {
	int i;
	if (step == 0 && H->collect !=0) {
		// finish last cycle
		do {
			collect(H);
		} while(H->collect != 0);
	} 
	if (H->collect == 0) {
		H->collect = HEAP_HEADER_SIZE;
	}

	for (i=0;step == 0 || i<step;i++) {
		collect(H);
		if (H->collect == 0) {
			break;
		}
	}
}

// for debug
#include <stdio.h>

void 
bpa_dump(struct bpa_heap * H) {
	printf("Heap size = %d K , %u used\n", H->sz * 8 / 1024, H->offset * 8);
	int i;
	for (i=0;i<MAX_SLOT;i++) {
		pointer_t pt = H->slot[i];
		if (pt) {
			printf("\tslot [%d] offset = 0x%x\n", i,pt);
		}
	}
	pointer_t offset = HEAP_HEADER_SIZE;
	while (offset < H->offset) {
		struct slice * s= HEAP_SLICE(H, offset);
		printf("\tslice [%d] offset = 0x%x ref = %d sz = %u\n",
			*SLICE_ID(s),
			offset,
			s->ref,
			s->sz * 8);
		offset += s->sz;
	}
}
