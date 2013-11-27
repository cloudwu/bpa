#include "bpa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void
set_id(struct bpa_heap *H, id uid, int v, size_t s) {
	char * buf = bpa_get_ptr(H, uid);
	memset(buf, v, s);
}

static void
dump_id(struct bpa_heap *H, id uid, size_t s) {
	printf("id = %x\n", uid);
	char * buf = bpa_get_ptr(H, uid);
	if (buf == NULL) {
		printf("not exist\n");
		return;
	}
	size_t i;
	for (i=0;i<s;i++) {
		printf("%x",buf[i]);
	}
	printf("\n");
}


int
main() {
	void * buffer = malloc(1024*1024);
	struct bpa_heap *H = bpa_init(buffer, 1024*1024);
	assert(H == buffer);
	bpa_dump(H);
	id id1 = bpa_create(H, 10000);
	id id2 = bpa_create(H, 20000);
	id id3 = bpa_create(H, 30000);
	bpa_create(H, 4000);
	assert(bpa_get_id(H, bpa_get_ptr(H,id1)) == id1);
	assert(bpa_get_id(H, bpa_get_ptr(H,id2)) == id2);
	assert(bpa_get_id(H, bpa_get_ptr(H,id3)) == id3);
	set_id(H, id1, 1, 10000);
	set_id(H, id2, 2, 20000);
	set_id(H, id3, 3, 30000);
	dump_id(H, id1, 100);
	dump_id(H, id2, 100);
	dump_id(H, id3, 100);
	dump_id(H, id1 + 0x10000, 100);

	bpa_dump(H);

	bpa_grab(H, id1);
	bpa_grab(H, id3);

	bpa_collect(H,1);
	bpa_dump(H);

	dump_id(H, id1, 100);
	dump_id(H, id2, 100);
	dump_id(H, id3, 100);

	bpa_release(H,id1);
	bpa_collect(H,0);
	bpa_dump(H);

	bpa_release(H,id3);
	bpa_collect(H,0);
	bpa_dump(H);

	return 0;
}
