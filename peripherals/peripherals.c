#include "peripherals.h"
#include "dma.h"
#include "timer.h"
#include "special.h"

#define PERIPH_BASE 0x04000000

void *periphInstructionResumeAddress;
uint32_t periphInstructionContent;
uint32_t periphThumb;
uint32_t lastPeripheralAddress;

uint32_t __attribute__((aligned(0x1000))) periphdata[PERIPH_SIZE >> 2];

void PeripheralsInit(void)
{
	size_t i;
	for(i = 0; i < PERIPH_SIZE >> 2; ++i)
		periphdata[i] = 0;

	MMUPopulateRange(PERIPH_BASE, (uint32_t) periphdata, PERIPH_SIZE, READONLY);

	for(i = 0; i < 0xFF; ++i)
		MMUPopulateRange(PERIPH_BASE + (i << 16), (uint32_t) periphdata, 0x1000, READONLY);
}

void PeripheralsSetAccess(enum AccessRights accessRights)
{
	__asm__ volatile("push {r0}\n"
					 "mrc p15, 0, r0, c1, c0, 0\n"
					 "bic r0, #0x1\n" // Disable MMU
					 "mcr p15, 0, r0, c1, c0, 0\n"
					 "mcr p15, 0, r0, c8, c7, 0\n" // Invalidate TLB entries
					 "pop {r0}\n"
					);

	MMUPopulateRange(lastPeripheralAddress & 0xFF000000, (uint32_t) periphdata, PERIPH_SIZE, accessRights);

	__asm__ volatile("push {r0}\n"
					 "mrc p15, 0, r0, c1, c0, 0\n"
					 "orr r0, #0x1\n" // Enable MMU
					 "mcr p15, 0, r0, c1, c0, 0\n"
					 "pop {r0}\n"
					);
}

void PeripheralsResume(void)
{
	if(periphThumb) // Thumb mode was enabled
		*(uint16_t*)periphInstructionResumeAddress = periphInstructionContent;
	else
		*(uint32_t*)periphInstructionResumeAddress = periphInstructionContent;
	PeripheralsSetAccess(READONLY);
}

void PeripheralsRefresh(void)
{
	//uint32_t addr = lastPeripheralAddress & ~((0xff<<16) | 0x3);
	
	//if(addr == 0x040000B8 || addr == 0x040000C4 || addr == 0x040000D0 || addr == 0x040000DC)
		DMARefresh();
	//else if(addr >= 0x04000100 && addr <= 0x0400010F)
		TimerRefresh();
	//else if(addr >= 0x04000200 && addr <= 0x04000804)
		PSpecialRefresh();
}

