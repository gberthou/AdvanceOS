#include "peripherals.h"
#include "dma.h"
#include "timer.h"
#include "special.h"
#include "../mem.h"

#define PERIPH_BASE 0x04000000

void *periphdata;

void PeripheralsInit(void)
{
    size_t i;

    periphdata = Memcalloc(PERIPH_SIZE, 0x1000);

    MMUPopulateRange((uint32_t) periphdata, (uint32_t) periphdata, PERIPH_SIZE, READWRITE); // map the physical mem
                                                                                            // so that periphdata
                                                                                            // can be used
    MMUPopulateRange(PERIPH_BASE, (uint32_t) periphdata, PERIPH_SIZE, READONLY); // map the virtual mem

    for(i = 0; i < 0xFF; ++i)
        MMUPopulateRange(PERIPH_BASE + (i << 16), (uint32_t) periphdata, 0x1000, READONLY);
}

void PeripheralsRefresh(void)
{
    //uint32_t addr = lastPeripheralAddress & ~((0xff<<16) | 0x3);
    
    //if(addr == 0x040000B8 || addr == 0x040000C4 || addr == 0x040000D0 || addr == 0x040000DC)
        DMARefresh();
    //else if(addr >= 0x04000100 && addr <= 0x0400010F)
        TimerRefresh();
    //else if(addr >= 0x04000200 && addr <= 0x04000804)
        PSpecialRefresh();
}

