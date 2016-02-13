#include <sys/types.h>

#include "dma.h"

#define DMA_BASE   0x20007000
#define DMA_ENABLE 0x20007FF0

#define DMA_CS_OFFSET        0x00
#define DMA_CONBLK_AD_OFFSET 0x01

#define DMA_PTR(channel, offset) (((volatile uint32_t*)DMA_BASE) + ((channel) << 6) + (offset))

struct DMAControlBlock
{
    uint32_t transferInfo;
    uint32_t srcAddress;
    uint32_t dstAddress;
    uint32_t transferLen;
    uint32_t stride;
    struct DMAControlBlock *next;
};

static void DMALaunchRequest(uint8_t channel, const struct DMAControlBlock *controlBlock)
{
    *(uint32_t*)DMA_ENABLE = (1 << channel); // Enable DMA channel
    *DMA_PTR(channel, DMA_CONBLK_AD_OFFSET) = (uint32_t) controlBlock;
    *DMA_PTR(channel, DMA_CS_OFFSET) = 7; // Enables DMA transfer
}

void DMACopy32(void *dst, void *src, size_t sizeBytes)
{
    static struct DMAControlBlock __attribute__((aligned(0x100))) controlBlock;

    controlBlock.transferInfo = (0 << 9)  // 32 bit source read
                              | (1 << 8)  // Increment source
                              | (0 << 5)  // 32 bit destination write
                              | (1 << 4); // Increment destination
    controlBlock.srcAddress = (uint32_t) src;
    controlBlock.dstAddress = (uint32_t) dst;
    controlBlock.transferLen = sizeBytes;
    controlBlock.stride = 0;
    controlBlock.next = 0; // No chained operation

    DMALaunchRequest(0, &controlBlock);
}

void DMAFillFramebuffer(void *dst, uint32_t value)
{
    static struct DMAControlBlock __attribute__((aligned(0x100))) controlBlock;
    static uint32_t val;

    val = value;

    controlBlock.transferInfo = (0 << 9)  // 32 bit source read
                              | (0 << 8)  // Do not increment source
                              | (0 << 5)  // 32 bit destination write
                              | (1 << 4)  // Increment destination
                              | (1 << 1); // Enable 2D mode
    controlBlock.srcAddress = (uint32_t) &val;
    controlBlock.dstAddress = (uint32_t) dst;
    controlBlock.transferLen = (240*4)      // Horizontal line size in bytes
                             | (159 << 16); // Number of horizontal lines
    controlBlock.stride = (240*4) << 16; // Leave debug space unchanged
    controlBlock.next = 0; // No chained operation

    DMALaunchRequest(0, &controlBlock);
}

