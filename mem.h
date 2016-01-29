#ifndef MEM_H
#define MEM_H

#include <sys/types.h>

void *Memalloc(size_t sizeBytes, size_t align);
void *Memcalloc(size_t sizeBytes, size_t align);

#endif

