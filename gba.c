#include <sys/types.h>

#include "gba.h"
#include "timer.h"
#include "irq.h"
#include "peripherals/peripherals.h"
#include "mem.h"
#include "linker.h"
#include "utils.h"

static uint32_t *alignedGbaRom;
static uint32_t *alignedGbaBios;

/*
static void testPermissions(void)
{
	uint32_t tmp;
	uint32_t *ptr;
	ptr = (uint32_t*) 0x4000000;
	tmp = *ptr; // Should work
	tmp |= 4;
	*ptr = tmp; // Should generate an exception

	for(;;);
}
*/

void GBALoadComponents(void)
{
	unsigned int i;

	alignedGbaBios = sysAlloc(GBA_BIOS_SIZE, 0x1000);
	alignedGbaRom = sysAlloc(GBA_ROM_SIZE, 0x1000);

	Copy32(alignedGbaBios, (void*)GBABIOS_BEGIN, 0, 0, GBABIOS_END - GBABIOS_BEGIN);
	Copy32(alignedGbaRom, (void*)GBAROM_BEGIN, 0, 0, GBAROM_END - GBAROM_BEGIN);
	
	MMUPopulateRange(0x00000000, (uint32_t) alignedGbaBios, GBA_BIOS_SIZE, READWRITE);
	
	for(i = 0; i < 3; ++i)	
		MMUPopulateRange(0x08000000 + i * 0x02000000, (uint32_t) alignedGbaRom, GBA_ROM_SIZE, READWRITE);
}

void GBARun(void)
{
	TimerLCDInit();
	IRQEnable();

	//testPermissions();

	__asm__ volatile("push {lr}\n"
					 "ldr pc, [pc, #0]\n"
					 "pop {lr}\n"
					 ".word 0x08000000\n");
}

void GBACallIRQ(void)
{
	__asm__ volatile("push {lr}\n"
					 "cps #0x12\n"
					 "add lr, pc, #4\n"
					 "b 0x18\n"
					 "pop {lr}");
}

