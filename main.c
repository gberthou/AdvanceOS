#include <stdio.h>

#include <sys/types.h>

#include <uspi.h>
//#include <uspienv.h>

#include "framebuffer.h"
#include "console.h"
#include "mmu.h"
#include "gba.h"
#include "peripherals/peripherals.h"
#include "irq.h"
#include "dma.h"
#include "mailbox.h"
#include "mem.h"
#include "uspienv/uspienv.h"

static void paintGreen(struct FBInfo *fb)
{
    size_t i;
    for(i = 0; i <240*160*2; ++i)
        fb->ptr[i] = 0xFF00FF00;
    FBCopyDoubleBuffer();
}

static void paintSpecial(struct FBInfo *fb)
{
    size_t x;
    size_t y;
    for(y = 0; y <160; ++y)
        for(x = 0; x < 480; ++x)
            fb->ptr[y*480 + x] = 0xFF000000 | (((x+y)&0xFF) * 0x010101);
    FBCopyDoubleBuffer();
}

int main(void)
{
    //USPiEnvInit(); 

    if(FBInit(240 * 2, 160))
    {
        struct FBInfo *fb;

        fb = FBCreateDoubleBuffer();
        paintGreen(fb);

        ConsolePrint(0, 0, "USB init...         ");
        FBCopyDoubleBuffer();
       
        /*
        if(!USPiInitialize())
        {
            ConsolePrint(20, 1, "FAIL");
            FBCopyDoubleBuffer();
            //USPiEnvClose();
            for(;;);
        }
        */
        /*
        
        ConsolePrint(20, 1, "OK");
        FBCopyDoubleBuffer();
       
        int n = USPiGamePadAvailable();
        ConsolePrintHex(0, 3, n);
        FBCopyDoubleBuffer();
        */

        PeripheralsInit();
        GBALoadComponents();
        FBConvertBufferToVirtualSpace();
        MMUInit();
        paintSpecial(fb);

        GBARun();
    }

    for(;;)
    {
    }

    return 0;
}

