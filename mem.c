#include "mem.h"
#include "linker.h"

// Heap size: 128 MB
#define HEAP_SIZE (128 << 20)

const uint8_t *heapHead = (uint8_t*) HEAP_BEGIN;
static uint8_t *heap = (uint8_t*) HEAP_BEGIN;

void *sysAlloc(size_t sizeBytes, size_t align)
{
	uint8_t *ret;
	--align;
	ret = (uint8_t*) ((((uint32_t)heap) + align) & ~align);

	if((uint32_t) ret >= HEAP_BEGIN + HEAP_SIZE - sizeBytes) // Out of heap :(
	{
		for(;;);
	}

	heap = ret + sizeBytes;
	return ret;
}

