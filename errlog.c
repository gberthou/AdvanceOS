#include <sys/types.h>

#include "framebuffer.h"
#include "console.h"

void ErrorDisplayMessage(const char *message)
{
    uint32_t x;
    uint32_t y;
    
    // First, put some aggressive color
    for(y = 0; y < 160; ++y)
        for(x = 0; x < 240*2; ++x)
            FBPutColor(x, y, 0xFF0000FF);
    ConsolePrint(1, 1, message);
    FBCopyDoubleBuffer();

    for(;;);
}

