#include <sys/types.h>

#include "mmu.h"
#include "linker.h"

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

/* Please see
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0301h/index.html
 */

#define MMU_N 0
#define FIRST_LEVEL_ALIGN (1 << (14 - MMU_N))
#define FIRST_LEVEL_COUNT (1 << (12 - MMU_N))
#define FIRST_LEVEL_OFFSET_MASK ((1 << (13 - MMU_N)) - 1)
#define FIRST_LEVEL_MASK (~FIRST_LEVEL_OFFSET_MASK)

#define SECOND_LEVEL_PER_ENTRY (1 << 8)
#define SECOND_LEVEL_COUNT (SECOND_LEVEL_PER_ENTRY * FIRST_LEVEL_COUNT)

static __attribute__((aligned(FIRST_LEVEL_ALIGN))) uint32_t firstEntries[FIRST_LEVEL_COUNT];
static uint32_t secondEntries[SECOND_LEVEL_COUNT];

// GBA Memories
static __attribute__((aligned(0x1000))) uint32_t wram0[WRAM0_SIZE>>2];
static __attribute__((aligned(0x1000))) uint32_t wram1[WRAM1_SIZE>>2];
static __attribute__((aligned(0x1000))) uint32_t paletteRam[PALETTE_RAM_SIZE>>2];
static __attribute__((aligned(0x1000))) uint32_t vram[VRAM_SIZE>>2];
static __attribute__((aligned(0x1000))) uint32_t oam[OAM_SIZE>>2];
static __attribute__((aligned(0x1000))) uint32_t sramGamePak[SRAM_GAMEPAK_SIZE>>2];

static void enableMMU(void)
{
	__asm__ volatile("push {r0}\n"
					 "mcr p15, 0, %0, c1, c0, 0\n" // Disable cache & MMU
					 "mcr p15, 0, r0, c8, c7, 0\n" // Invalidate Unified TLB entries 
					 "mcr p15, 0, %1, c1, c0, 0\n"
					 "pop {r0}\n"
					 :: "r"(0),
						"r"(0x00802071)// MMU Enabled | Interrupt vector base @0xFFFF0000 | subpage AP bits enabled (->ARMv6 format)
					 );
}

static void initTableBaseControlRegister(void)
{ 
	__asm__ volatile("push {r0}\n"
					 "mov r0, %0\n"
					 "mcr p15, 0, r0, c2, c0, 2\n"
					 "pop {r0}\n"
					 :: "r"(MMU_N));
}

static void initTranslationTableBaseRegisters(void)
{
	__asm__ volatile("push {r0}\n"

					 // Register0
					 "mov r0, %0\n"
					 "mcr p15, 0, r0, c2, c0, 0\n" 
					 
					 // Register1
					 "mcr p15, 0, r0, c2, c0, 1\n"

					 "pop {r0}\n"
					 :: "r"(firstEntries));
}

static void initPermissions(void)
{
	__asm__ volatile("push {r0}\n"
					 "mov r0, #0x1\n" // Enables permission check for domain D0
					 "mcr p15, 0, r0, c3, c0, 0\n"
					 "pop {r0}\n");
}

static uint32_t *getFirstLevelEntry(uint32_t virtualAddress)
{
	return firstEntries + (virtualAddress >> 20);
}

static uint32_t *getSecondLevelEntry(uint32_t virtualAddress)
{
	uint32_t firstOffset = (virtualAddress >> 20);
	return secondEntries + firstOffset * SECOND_LEVEL_PER_ENTRY + ((virtualAddress >> 12) & 0xFF);
}

void MMUPopulateRange(uint32_t vAddress, uint32_t pAddress, size_t size, enum AccessRights accessRights)
{
	const uint32_t LVL2_MASK = (1 << 12) - 1;
	uint32_t *fEntry;
	uint32_t maxVAddress = (vAddress + size - 1 + LVL2_MASK) & ~LVL2_MASK;
	uint32_t actualVAddress;

		/* Please see
		 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0301h/index.html
		 *
		 * TEX: 000
		 * AP : 3 (same access rights for user and supervisor)
		 * C  : 0
		 * B  : 0
		 *
		 * 4KB extended small page translations, ARMv6 format
		 */
	uint32_t secondEntrySuffix = (3 << 4) | 0x2 | (accessRights == READONLY ? (1 << 9) : 0);

	for(actualVAddress = vAddress & ~LVL2_MASK;
		actualVAddress < maxVAddress;
		actualVAddress += (1 << 12),
		pAddress += (1 << 12))
	{
		uint32_t *sEntry = getSecondLevelEntry(actualVAddress);
		fEntry = getFirstLevelEntry(actualVAddress);
		*fEntry = (((uint32_t)sEntry) & 0xFFFFFC00) | 0x1;

		*sEntry = (pAddress & 0xFFFFF000) | secondEntrySuffix;
	}
}

/*
 * Test purposes only
uint32_t virt2Physical(uint32_t virt)
{
	uint32_t *sEntry = getSecondLevelEntry(virt);

	return (*sEntry & 0xFFFFF000) | (virt & 0x00000FFF);
}
*/

void MMUInit(void)
{
	initTableBaseControlRegister();
	initTranslationTableBaseRegisters();
	initPermissions();
	
	// First, make the kernel virtual addresses equal to the physical ones
	MMUPopulateRange(KERNEL_TEXT_BEGIN, KERNEL_TEXT_BEGIN, KERNEL_TEXT_SIZE, READWRITE);
	MMUPopulateRange(KERNEL_DATA_BEGIN, KERNEL_DATA_BEGIN, KERNEL_DATA_SIZE, READWRITE);
	
	// Do the same with the stacks
	MMUPopulateRange(STACK_BEGIN, STACK_BEGIN, KERNEL_TEXT_BEGIN - STACK_BEGIN, READWRITE);

	// Do the same with raspberry's peripherals
	MMUPopulateRange(0x20000000, 0x20000000, 0x01000000, READWRITE);

	// Set translation table for interrupt vector
	MMUPopulateRange(0xFFFF0000, VECTORTABLE_BEGIN, VECTORTABLE_SIZE, READWRITE);

	// Map GBA memories
	MMUPopulateRange(WRAM0_BEGIN, (uint32_t) wram0, WRAM0_SIZE, READWRITE);
	MMUPopulateRange(WRAM1_BEGIN, (uint32_t) wram1, WRAM1_SIZE, READWRITE);
	MMUPopulateRange(PALETTE_RAM_BEGIN, (uint32_t) paletteRam, PALETTE_RAM_SIZE, READWRITE);
	MMUPopulateRange(VRAM_BEGIN, (uint32_t) vram, VRAM_SIZE, READWRITE);
	MMUPopulateRange(OAM_BEGIN, (uint32_t) oam, OAM_SIZE, READWRITE);
	MMUPopulateRange(SRAM_GAMEPAK_BEGIN, (uint32_t) sramGamePak, SRAM_GAMEPAK_SIZE, READWRITE);

	// Map GBA mirrored memories
	MMUPopulateRange(WRAM1_BEGIN + 0x00FFF000, ((uint32_t) wram1) + 0x00007000, 0x1000, READWRITE);

	enableMMU();
}

