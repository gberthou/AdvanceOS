#include "timer.h"
#include "interrupt.h"
#include "alloc.h"

void USPiEnvInit(void)
{
    USPiEnvAllocInit();
    USPiEnvTimerInit();
    USPiEnvInterruptInit();
}

