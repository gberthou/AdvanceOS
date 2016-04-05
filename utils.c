#include "utils.h"

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

