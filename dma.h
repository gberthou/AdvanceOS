#ifndef DMA_H
#define DMA_H

void DMAInit(void);
void DMACopy32(void *dst, void *src, size_t sizeBytes);

#endif

