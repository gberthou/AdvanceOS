#include <sys/types.h>

#include "../mem.h"
#include "../linker.h"
#include "../errlog.h"

// heap size: 64 MB
#define HEAP_BEGIN (GBA_HEAP_BEGIN + GBA_HEAP_SIZE)
#define HEAP_SIZE (64 << 20)
#define MEMBLOCK_COUNT 1024

struct MemBlock
{
    void *ptr;
    size_t size;
    struct MemBlock *next;
};

static struct MemBlock *freeblocks;      // List of free blocks
static struct MemBlock *allocatedblocks; // List of allocated blocks

static struct MemBlock blockpool[MEMBLOCK_COUNT];
static uint8_t blockavailable[MEMBLOCK_COUNT >> 3]; // 1 bit <-> 1 block, so it needs 8 times less cells

static struct MemBlock *allocMemBlock(void)
{
    size_t i;
    for(i = 0; i < (MEMBLOCK_COUNT >> 3); ++i)
    {
        if(blockavailable[i] != 0xFF) // At least one block is available in this cell
        {
            size_t j;
            for(j = 0; j < 8 && (blockavailable[i] & (1 << j)); ++j);
            // The previous if statement ensures that 0 <= j <= 7
            blockavailable[i] |= (1 << j);
            return blockpool + (i << 3) + j;
        }
    }
    return 0; // No available block
}

/*
static void freeMemBlock(const struct MemBlock *block)
{
    size_t index = block - blockpool;
    blockavailable[index >> 3] &= ~(1 << (index & 0x7));
}
*/

void USPiEnvAllocInit(void)
{
    size_t i;

    for(i = 0; i < (MEMBLOCK_COUNT >> 3); ++i)
        blockavailable[i] = 0;

    freeblocks = allocMemBlock();
    if(freeblocks == 0)
    {
        ErrorDisplayMessage("MemInit: no free block");
    }

    freeblocks->ptr = (void*)HEAP_BEGIN;
    freeblocks->size = HEAP_SIZE;
    freeblocks->next = 0;

    allocatedblocks = 0;
}

static void insertAllocatedBlock(struct MemBlock *memblock)
{
    memblock->next = allocatedblocks;
    allocatedblocks = memblock;
}

static void removeAllocatedBlock(struct MemBlock *memblock, struct MemBlock *prec)
{
    if(prec)
        prec->next = memblock->next;
    else
        allocatedblocks = memblock->next;
}

static void insertFreeBlock(struct MemBlock *memblock)
{
    memblock->next = freeblocks;
    freeblocks = allocatedblocks;
}

static void removeFreeBlock(struct MemBlock *memblock, struct MemBlock *prec)
{
    if(prec)
        prec->next = memblock->next;
    else
        freeblocks = memblock->next;
}

void *malloc(size_t sizeBytes)
{
    struct MemBlock *memblock;
    struct MemBlock *prec = 0;
    for(memblock = freeblocks; memblock; prec = memblock, memblock = memblock->next)
    {
        if(memblock->size == sizeBytes) // Perfect fitting
        {
            void *ret = memblock->ptr;

            removeFreeBlock(memblock, prec);

            insertAllocatedBlock(memblock);
            return ret;
        }
        if(memblock->size > sizeBytes) // Memory block cutting required
        {
            struct MemBlock *newblock = allocMemBlock();

            if(!newblock)
            {
                ErrorDisplayMessage("malloc: no free block");
            }
            
            newblock->ptr = memblock->ptr;
            newblock->size = sizeBytes;
            
            memblock->ptr = (void*)(((uint32_t)memblock->ptr) + sizeBytes);
            memblock->size -= sizeBytes;
            
            insertAllocatedBlock(newblock);

            return newblock->ptr;
        }
    }
    return 0;
}

void free(void *ptr)
{
    struct MemBlock *it = allocatedblocks;
    struct MemBlock *prec = 0;
    if(it)
    {
        while((ptr < it->ptr || (uint32_t)ptr >= (uint32_t)it->ptr + it->size) && it->next)
        {
            prec = it;
            it = it->next;
        }

        if(ptr >= it->ptr && (uint32_t)ptr < (uint32_t)it->ptr + it->size)
        {
            removeAllocatedBlock(it, prec);
            insertFreeBlock(it);
        }
        else // Not found
        {
            ErrorDisplayMessage("free: block not found");
        }
    }
    else // There is no allocated block
    {
        ErrorDisplayMessage("free: calling free when nothing is allocated yet");
    }
}

