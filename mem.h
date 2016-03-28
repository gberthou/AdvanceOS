#ifndef MEM_H
#define MEM_H

#include <sys/types.h>

#define GBA_HEAP_BEGIN_VIRTUAL 0x30000000
#define VirtualHeapAddressOf(x) (((uint32_t)(x)) - GBA_HEAP_BEGIN \
                                + GBA_HEAP_BEGIN_VIRTUAL)
#define PhysicalHeapAddressOf(x) (((uint32_t)(x)) + GBA_HEAP_BEGIN \
                                - GBA_HEAP_BEGIN_VIRTUAL)

// GBA heap size: 8 MB
#define GBA_HEAP_SIZE (8 << 20)

#define MemallocVirtual(sizeBytes) ((void*)VirtualHeapAddressOf(\
                                   Memalloc(sizeBytes, 0x1000)))
#define MemcallocVirtual(sizeBytes) ((void*)VirtualHeapAddressOf(\
                                    Memcalloc(sizeBytes, 0x1000)))

void MemInit(void);
void *Memalloc(size_t sizeBytes, size_t align);
void *Memcalloc(size_t sizeBytes, size_t align);
void *MemallocMapped(size_t sizeBytes, size_t align);

#endif

