#include <stdio.h>

#include <sys/types.h>

#include <uspi.h>
#include <uspios.h>
#include <uspi/string.h>
#include <uspi/usbxbox360.h>

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
#include "errlog.h"
#include "linker.h"
#include "uspienv/alloc.h"
#include "uspienv/logger.h"
#include "usb.h"
#include "timer.h"

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

static void GamePadStatusHandler (unsigned int nDeviceIndex, const USPiGamePadState *pState)
{
    (void)nDeviceIndex;

    ConsolePrintHex(32, 18, pState->buttons);
    ConsolePrintHex(32, 19, pState->axes[4].value);
}

static void USPiInit(void)
{
    ConsolePrint(0, 0, "USB init...         ");
    FBCopyDoubleBuffer();

    if(!USPiInitialize())
        ErrorDisplayMessage("USPiInitialize: cannot init USB", 1);

    ConsolePrint(20, 0, "OK");
    FBCopyDoubleBuffer();

    if(USPiGamePadAvailable())
    {
        USPiGamePadRegisterStatusHandler(GamePadStatusHandler);
        IRQEnable();
    }
}

int main(void)
{
    MemInit();
    USPiEnvInit(); 
    TimerInit();

    if(FBInit(240 * 2, 160))
    {
        struct FBInfo *fb;

        fb = FBCreateDoubleBuffer();
        paintGreen(fb);

        PeripheralsInit();
        GBALoadComponents();
        FBConvertBufferToVirtualSpace();
        MMUInit();
        
        USPiInit();

        paintSpecial(fb);
        GBARun();
    }

    for(;;);

    return 0;
}

