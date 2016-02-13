#ifndef DMA_H
#define DMA_H

void DMAInit(void);
void DMACopy32(void *dst, void *src, size_t sizeBytes);
void DMAFillFramebuffer(void *dst, uint32_t value);

#endif

