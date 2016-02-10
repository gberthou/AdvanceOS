#include <sys/types.h>

#include "dma.h"
#include "peripherals.h"
#include "../utils.h"
#include "../console.h"

void DMARefreshChannel(unsigned int channel)
{
    volatile uint32_t *base = PERIPH32(0xB0 + 0xc * channel);
    uint32_t control = base[2];

    // TODO: Manage DMA Start Timing bits, etc.
    if(control & (1 << 31)) // Channel enabled
    {
        uint32_t startTiming = (control >> 28) & 3;
        if(startTiming == 0) // Start immediatly
        {
            if(control & (1 << 26)) // 32 bit transfer
                Copy32((uint32_t*) base[1], (uint32_t*) base[0], (control >> 21) & 3, (control >> 23) & 3, control & 0xFFFF);
            else // 16 bit transfer
                Copy16((uint16_t*) base[1], (uint16_t*) base[0], (control >> 21) & 3, (control >> 23) & 3, control & 0xFFFF);
        }
        else if(startTiming == 3) // Start special
        {
            if(channel == 1 || channel == 2) // Sound FIFO
            {
                // Transfer 16 bytes while keeping dest address constant
                Copy32((uint32_t*) base[1], (uint32_t*) base[0], 2, (control >> 23) & 3, 16);
            }
            else if(channel == 3) // Video capture
            {
                ConsolePrint(31, 9, "DMA video capture transfer not implemented");
            }
            else
            {
                ConsolePrint(31, 1, "DMA transfer prohibited (special on channel 0)");
                for(;;);
            }
        }
        else
        {
            ConsolePrint(31, 9, "DMA channel: ");
            ConsolePrintHex(44, 9, channel);
            ConsolePrint(31, 10, "??? ");
            ConsolePrintHex(35, 10, (control >> 28) & 3);
        }

        if((control & (1 << 25)) == 0) // No repeat
            base[2] &= 0x7FFFFFFF; // Clear channel enable bit
    }
}

void DMARefresh(void)
{
    unsigned int i;
    for(i = 0; i < 4; ++i)
        DMARefreshChannel(i);
}

