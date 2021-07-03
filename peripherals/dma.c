#include <stdint.h>
#include <stddef.h>

#include "dma.h"
#include "peripherals.h"
#include "../utils.h"
#include "../console.h"
#include "../errlog.h"

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

static void copy32(uint32_t *dst, const uint32_t *src, uint32_t dstDecr,
                   uint32_t srcDecr, size_t nUnits)
{
    int32_t dstIncrement = increment(dstDecr);
    int32_t srcIncrement = increment(srcDecr);

    // TODO: Use DMA instead
    while(nUnits--)
    {
        *dst = *src;
        dst += dstIncrement;
        src += srcIncrement;
    }
}

static void copy16(uint16_t *dst, const uint16_t *src, uint32_t dstDecr,
                   uint32_t srcDecr, size_t nUnits)
{
    int32_t dstIncrement = increment(dstDecr);
    int32_t srcIncrement = increment(srcDecr);
    
    while(nUnits--)
    {
        *dst = *src;
        dst += dstIncrement;
        src += srcIncrement;
    }
}

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
            uint32_t dst;

            if(channel == 3)
            {
                transfersize = (control & 0xFFFF);
                if(!transfersize)
                    transfersize = 0x10000;
            }
            else
            {
                transfersize = (control & 0x3FFF);
                if(!transfersize)
                    transfersize = 0x4000;
            }

            if(base[1] >= PERIPH_BASE && base[1] < PERIPH_BASE + PERIPH_SIZE)
            {
                if(   base[1] >= PERIPH_BASE + 0x90
                   && base[1] <  PERIPH_BASE + 0xA8) // WAVE_RAM, FIFO_A,
                                                     // FIFO_B
                {
                    dst = ((uint32_t)periphdata) + base[1] - PERIPH_BASE;
                }
                else
                {
                    ErrorDisplayMessage("GBA DMA requested within invalid "
                                        "peripheral range", 1);
                    return;
                }
            }
            else
                dst = base[1];

            if(control & (1 << 26)) // 32 bit transfer
                copy32((uint32_t*) dst, (uint32_t*) base[0],
                       (control >> 21) & 3, (control >> 23) & 3, transfersize);
            else // 16 bit transfer
                copy16((uint16_t*) dst, (uint16_t*) base[0],
                       (control >> 21) & 3, (control >> 23) & 3, transfersize);
        }
        else if(startTiming == 3) // Start special
        {
            if(channel == 1 || channel == 2) // Sound FIFO
            {
                if(   base[1] == PERIPH_BASE + 0xA0  // FIFO_A
                   || base[1] == PERIPH_BASE + 0xA4) // FIFO_B
                {
                    uint32_t *dst = (uint32_t*)(((uint32_t)periphdata)
                                    + base[1] - PERIPH_BASE);
                    // Transfer 16 bytes while keeping dest address constant
                    copy32(dst, (uint32_t*) base[0], 2, (control >> 23) & 3,
                           4);
                    // TODO: Update sound
                }
                else
                    ErrorDisplayMessage("GBA DMA requested for Sound FIFO "
                                        "transfer with invalid dest address",
                                        1);
            }
            else if(channel == 3) // Video capture
            {
                ErrorDisplayMessage("DMA video capture transfer not implemented"
                                    , 1);
            }
            else
            {
                ErrorDisplayMessage("DMA transfer prohibited (special on "
                                    "channel 0)", 1);
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

