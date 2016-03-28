#include <sys/types.h>

#include "mem.h"
#include "linker.h"
#include "errlog.h"

static uint8_t *gbaHeap;

void MemInit(void)
{
    gbaHeap = (uint8_t*) GBA_HEAP_BEGIN;
}

void *Memalloc(size_t sizeBytes, size_t align)
{
    uint8_t *ret;
    --align;
    ret = (uint8_t*) ((((uint32_t)gbaHeap) + align) & ~align);

    if((uint32_t) ret >= GBA_HEAP_BEGIN + GBA_HEAP_SIZE - sizeBytes) // Out of heap :(
    {
        ErrorDisplayMessage("Memalloc: Out of heap", 1);
    }

    gbaHeap = ret + sizeBytes;
    return ret;
}

void *Memcalloc(size_t sizeBytes, size_t align)
{
    uint8_t *ret = Memalloc(sizeBytes, align);
    size_t i;

    // TODO: Optimize this (DMA and/or using words instead of bytes)
    for(i = 0; i < sizeBytes; ++i)
        ret[i] = 0;

    return ret;
}

