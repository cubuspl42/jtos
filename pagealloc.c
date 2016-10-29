#include "pagealloc.h"
#include "console.h"
#include "paging.h"

#define PAGE_POOL_SIZE 32

static char page_pool[PAGE_POOL_SIZE][PAGE_SIZE] PAGE_ALIGNED;
static int page_pool_index = 0;

static void *memset(void *dest, int e, unsigned long len) {
    uint8_t *d = dest;
    for(uint64_t i = 0; i < len; i++, d++) {
        *d = e;
    }
    return dest;
}

void *pgalloc()
{
	// kassert(page_pool_index < PAGE_POOL_SIZE);
	void *page = page_pool[page_pool_index++];
	memset(page, 0, PAGE_SIZE);
	return page;
}
