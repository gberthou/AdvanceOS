#include "utils.h"

static int32_t increment(uint32_t code)
{
    switch(code)
    {
        case 1:
            return -1;
        case 2:
            return 0;
        default:
            return 1;
    }
}

void Copy32(uint32_t *dst, const uint32_t *src, uint32_t dstDecr, uint32_t srcDecr, size_t byteCount)
{
    int32_t dstIncrement = increment(dstDecr);
    int32_t srcIncrement = increment(srcDecr);

    // TODO: Use DMA instead
    byteCount >>= 2;
    while(byteCount--)
    {
        *dst = *src;
        dst += dstIncrement;
        src += srcIncrement;
    }
}

void Copy16(uint16_t *dst, const uint16_t *src, uint32_t dstDecr, uint32_t srcDecr, size_t byteCount)
{
    int32_t dstIncrement = increment(dstDecr);
    int32_t srcIncrement = increment(srcDecr);

    byteCount >>= 1;
    while(byteCount--)
    {
        *dst = *src;
        dst += dstIncrement;
        src += srcIncrement;
    }
}

int Test32(uint32_t *a, uint32_t *b, size_t byteCount)
{
    byteCount >>= 2;
    while(byteCount--)
    {
        if(*a++ != *b++)
            return 0;
    }
    return 1;
}

