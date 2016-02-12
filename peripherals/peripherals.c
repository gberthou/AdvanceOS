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

void PeripheralsSetAccess(enum AccessRights accessRights)
{
    __asm__ volatile("push {r0}\n"
                     "mrc p15, 0, r0, c1, c0, 0\n"
                     "bic r0, #0x1\n" // Disable MMU
                     "mcr p15, 0, r0, c1, c0, 0\n"
                     "mcr p15, 0, r0, c8, c7, 0\n" // Invalidate TLB entries
                     "pop {r0}\n"
                    );

    MMUPopulateRange(PERIPH_BASE, (uint32_t) periphdata, PERIPH_SIZE, accessRights);

    __asm__ volatile("push {r0}\n"
                     "mrc p15, 0, r0, c1, c0, 0\n"
                     "orr r0, #0x1\n" // Enable MMU
                     "mcr p15, 0, r0, c1, c0, 0\n"
                     "pop {r0}\n"
                    );
}


void PeripheralsRefresh(uint32_t address)
{
    if(address == 0x040000B8
    || address == 0x040000C4
    || address == 0x040000D0
    || address == 0x040000DC)
        DMARefresh();
    else if(address >= 0x04000100 && address <= 0x0400010F)
        TimerRefresh();
    else if(address >= 0x04000200 && address <= 0x04000804)
        PSpecialRefresh();
}

