#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

void Copy32(uint32_t *dst, const uint32_t *src, uint32_t dstDecr, uint32_t srcDecr, size_t nUnits);
void Copy16(uint16_t *dst, const uint16_t *src, uint32_t dstDecr, uint32_t srcDecr, size_t nUnits);

// Returns 1 if a and b have the same data (over byteCount bytes)
//         0 otherwise
int Test32(uint32_t *a, uint32_t *b, size_t byteCount);

#endif

