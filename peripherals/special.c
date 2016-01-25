#include "special.h"
#include "peripherals.h"
#include "../console.h"

void PSpecialInit(void)
{
}

void PSpecialRefresh(void)
{
	ConsolePrint(31, 3, "IME:");
	ConsolePrintHex(36,3, periphdata[0x208>>2]);

	ConsolePrint(31, 4, "IE :");
	ConsolePrintHex(36, 4, periphdata[0x200>>2]);
}

