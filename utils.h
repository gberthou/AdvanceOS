#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

// Returns 1 if a and b have the same data (over byteCount bytes)
//         0 otherwise
int Test32(uint32_t *a, uint32_t *b, size_t byteCount);

#endif

