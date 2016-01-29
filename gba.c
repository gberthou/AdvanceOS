#include <sys/types.h>

#include "gba.h"
#include "timer.h"
#include "irq.h"
#include "peripherals/peripherals.h"
#include "mem.h"
#include "linker.h"
#include "utils.h"

#define WRAM0_BEGIN 0x02000000
#define WRAM0_SIZE 0x40000

#define WRAM1_BEGIN 0x03000000
#define WRAM1_SIZE 0x8000

#define PALETTE_RAM_BEGIN 0x05000000
#define PALETTE_RAM_SIZE 0x400

#define VRAM_BEGIN 0x06000000
#define VRAM_SIZE 0x18000

#define OAM_BEGIN 0x07000000
#define OAM_SIZE 0x400

#define SRAM_GAMEPAK_BEGIN 0x0E000000
#define SRAM_GAMEPAK_SIZE 0x10000

void GBALoadComponents(void)
{
	unsigned int i;

	void *alignedGbaBios = Memalloc(GBA_BIOS_SIZE, 0x1000);
	void *alignedGbaRom = Memalloc(GBA_ROM_SIZE, 0x1000);

	void *wram0       = Memcalloc(WRAM0_SIZE, 0x1000);
	void *wram1       = Memcalloc(WRAM1_SIZE, 0x1000);
	void *paletteRam  = Memcalloc(PALETTE_RAM_SIZE, 0x1000);
	void *vram        = Memcalloc(VRAM_SIZE, 0x1000);
	void *oam         = Memcalloc(OAM_SIZE, 0x1000);
	void *sramGamePak = Memcalloc(SRAM_GAMEPAK_SIZE, 0x1000);

	Copy32(alignedGbaBios, (void*)GBABIOS_BEGIN, 0, 0, GBABIOS_END - GBABIOS_BEGIN);
	Copy32(alignedGbaRom, (void*)GBAROM_BEGIN, 0, 0, GBAROM_END - GBAROM_BEGIN);
	
	// Map BIOS
	MMUPopulateRange(0x00000000, (uint32_t) alignedGbaBios, GBA_BIOS_SIZE, READWRITE);
	
	// Map ROM and its mirrors
	for(i = 0; i < 3; ++i)	
		MMUPopulateRange(0x08000000 + i * 0x02000000, (uint32_t) alignedGbaRom, GBA_ROM_SIZE, READWRITE);
	
	// Map GBA memories
	MMUPopulateRange(WRAM0_BEGIN, (uint32_t) wram0, WRAM0_SIZE, READWRITE);
	MMUPopulateRange(WRAM1_BEGIN, (uint32_t) wram1, WRAM1_SIZE, READWRITE);
	MMUPopulateRange(PALETTE_RAM_BEGIN, (uint32_t) paletteRam, PALETTE_RAM_SIZE, READWRITE);
	MMUPopulateRange(VRAM_BEGIN, (uint32_t) vram, VRAM_SIZE, READWRITE);
	MMUPopulateRange(OAM_BEGIN, (uint32_t) oam, OAM_SIZE, READWRITE);
	MMUPopulateRange(SRAM_GAMEPAK_BEGIN, (uint32_t) sramGamePak, SRAM_GAMEPAK_SIZE, READWRITE);

	// Map GBA mirrored memories
	MMUPopulateRange(WRAM1_BEGIN + 0x00FFF000, ((uint32_t) wram1) + 0x00007000, 0x1000, READWRITE);

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

