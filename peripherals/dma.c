#include <sys/types.h>

#include "dma.h"
#include "peripherals.h"
#include "../utils.h"
#include "../console.h"
#include "../errlog.h"

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
            uint32_t transfersize;

            if(channel == 3)
            {
                transfersize = (control & 0xFFFF) + 1;
                if(transfersize <= 1)
                    transfersize = 0x10000;
            }
            else
            {
                transfersize = (control & 0x3FFF) + 1;
                if(transfersize <= 1)
                    transfersize = 0x4000;
            }

            if(control & (1 << 26)) // 32 bit transfer
                Copy32((uint32_t*) base[1], (uint32_t*) base[0], (control >> 21) & 3, (control >> 23) & 3, transfersize);
            else // 16 bit transfer
                Copy16((uint16_t*) base[1], (uint16_t*) base[0], (control >> 21) & 3, (control >> 23) & 3, transfersize);
        }
        else if(startTiming == 3) // Start special
        {
            ErrorDisplayMessage("dma special", 1);
            if(channel == 1 || channel == 2) // Sound FIFO
            {
                // Transfer 16 bytes while keeping dest address constant
                Copy32((uint32_t*) base[1], (uint32_t*) base[0], 2, (control >> 23) & 3, 16);
            }
            else if(channel == 3) // Video capture
            {
                ErrorDisplayMessage("DMA video capture transfer not implemented", 1);
            }
            else
            {
                ErrorDisplayMessage("DMA transfer prohibited (special on channel 0)", 1);
            }
        }
        else
        {
            ConsolePrint(31, 9, "DMA channel: ");
            ConsolePrintHex(44, 9, channel);
            ConsolePrint(31, 10, "??? ");
            ConsolePrintHex(35, 10, (control >> 28) & 3);
            for(;;);
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

