#ifndef MEM_H
#define MEM_H

#include <sys/types.h>

// GBA heap size: 64 MB
#define GBA_HEAP_SIZE (64 << 20)

void *Memalloc(size_t sizeBytes, size_t align);
void *Memcalloc(size_t sizeBytes, size_t align);

#endif

