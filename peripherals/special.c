#include "special.h"
#include "peripherals.h"
#include "../console.h"

void PSpecialInit(void)
{
}

void PSpecialRefresh(void)
{
    ConsolePrint(31, 3, "IME:");
    ConsolePrintHex(36,3, *PERIPH32(0x208));

    ConsolePrint(31, 4, "IE :");
    ConsolePrintHex(36, 4, *PERIPH32(0x200));
}

