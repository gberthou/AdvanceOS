#include <stdio.h>

#include <sys/types.h>

#include "framebuffer.h"
#include "console.h"
#include "mmu.h"
#include "gba.h"
#include "peripherals/peripherals.h"
#include "irq.h"

void outputOK(void)
{
	volatile uint32_t *GPFSEL2 = (uint32_t*) 0x20200008;
	uint32_t *GPSET0 = (uint32_t*) 0x2020001C;

	*GPFSEL2 = (1 << 3);
	*GPSET0 = (1 << 21);
}

static void paintGreen(void)
{
	size_t i;
	for(i = 0; i <240*160*2; ++i)
		fbInfo.ptr[i] = 0xFF00FF00;
}

static void paintSpecial(void)
{
	size_t x;
	size_t y;
	for(y = 0; y <160; ++y)
		for(x = 0; x < 480; ++x)
			fbInfo.ptr[y*480 + x] = 0xFF000000 | (((x+y)&0xFF) * 0x010101);
}

int main(void)
{
	if(FBInit(240 * 2, 160))
	{
		paintGreen();
		
		GBALoadComponents();
		FBConvertBufferToVirtualSpace();
		PeripheralsInit();
		MMUInit();

		paintSpecial();
		GBARun();
	}
	for(;;)
	{
	}

	return 0;
}

