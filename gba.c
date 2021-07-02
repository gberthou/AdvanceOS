#include <sys/types.h>

#include "gba.h"
#include "timer.h"
#include "irq.h"
#include "peripherals/peripherals.h"
#include "mem.h"
#include "linker.h"
#include "utils.h"
#include "swifunctions.h"
#include "dma.h"

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

static void *wram1;

static void patchBios(uint32_t *biosdata)
{
    // Replace GBA SVC vector entry by System SVC vector entry
    uint32_t *gbaSvcVector = biosdata + (0x1c8 >> 2);
    
    /* Patch custom bios functions */
    // Replace IntrWait (svc 0x04)
    gbaSvcVector[4] = (uint32_t)SWI_IntrWait;

    // Replace VBlankIntrWait (svc 0x05)
    gbaSvcVector[5] = (uint32_t)SWI_VBlankIntrWait;


    // Replace "return from interrupt" instruction of IRQ handler
    biosdata[0x13c >> 2] = 0xe12fff1e; // bx lr
}

void GBALoadComponents(void)
{
    unsigned int i;

    void *alignedGbaBios = Memalloc(GBA_BIOS_SIZE, 0x1000);

    void *wram0       = Memcalloc(WRAM0_SIZE, 0x1000);
          wram1       = Memcalloc(WRAM1_SIZE, 0x1000);
    void *paletteRam  = Memcalloc(PALETTE_RAM_SIZE, 0x1000);
    void *vram        = Memcalloc(VRAM_SIZE, 0x1000);
    void *oam         = Memcalloc(OAM_SIZE, 0x1000);
    void *sramGamePak = Memcalloc(SRAM_GAMEPAK_SIZE, 0x1000);

    DMACopy32(alignedGbaBios, (void*)GBABIOS_BEGIN,
              (GBABIOS_END - GBABIOS_BEGIN) >> 2);
    
    // Code modifications
    patchBios(alignedGbaBios);

    // Map BIOS
    MMUPopulateRange(0x00000000, (uint32_t) alignedGbaBios, GBA_BIOS_SIZE,
                     READWRITE);
    
    // Map ROM and its mirrors
    for(i = 0; i < 3; ++i)    
        MMUPopulateRange(0x08000000 + i * 0x02000000, (uint32_t) GBAROM_BEGIN,
                         GBA_ROM_SIZE, READWRITE);
    
    // Map GBA memories
    MMUPopulateRange(WRAM0_BEGIN, (uint32_t) wram0, WRAM0_SIZE, READWRITE);

#if 0
    // WRAM1
    // 1. The "first part" of WRAM1 should be accessible at any time
    MMUPopulateRange(WRAM1_BEGIN, (uint32_t) wram1, WRAM1_SIZE - 0x1000,
                     READWRITE);
    // 2. The last page of WRAM1 should be accessible via identity-mapped
    //    address (system purpose)
    MMUPopulateRange(((uint32_t)wram1) + 0x7000, ((uint32_t) wram1) + 0x7000,
                     0x1000, READWRITE);
    // 3. The last page of WRAM1 should be readonly to the user so that an
    //    interrupt is generated when user code tries to write to 0x3007FF8
    MMUPopulateRange(WRAM1_BEGIN + 0x7000, ((uint32_t) wram1) + 0x7000, 0x1000,
                     READONLY);
#else
    MMUPopulateRange(WRAM1_BEGIN, (uint32_t) wram1, WRAM1_SIZE, READWRITE);
    MMUPopulateRange(((uint32_t)wram1), ((uint32_t)wram1), WRAM1_SIZE,
                     READWRITE);
#endif

    MMUPopulateRange(PALETTE_RAM_BEGIN, (uint32_t) paletteRam, PALETTE_RAM_SIZE,
                     READWRITE);
    MMUPopulateRange(VRAM_BEGIN, (uint32_t) vram, VRAM_SIZE, READWRITE);
    MMUPopulateRange(OAM_BEGIN, (uint32_t) oam, OAM_SIZE, READWRITE);
    MMUPopulateRange(SRAM_GAMEPAK_BEGIN, (uint32_t) sramGamePak,
                     SRAM_GAMEPAK_SIZE, READWRITE);

    // Map GBA mirrored memories
    MMUPopulateRange(WRAM1_BEGIN + 0x00FFF000, ((uint32_t) wram1) + 0x00007000,
                     0x1000, READONLY);
}

void GBARun(void)
{
    //KeypadInit();
    //KeypadReset();

    TimerEnableLCD();
    IRQEnable();

    __asm__ volatile("push {lr}\n"
                     "ldr pc, =#0x08000000\n"
                     "pop {lr}\n");
}

static void synchronizeInterruptFlags(void)
{
    // Clear the flag which were requested by user IRQ handler
    *PERIPH16(0x202) &= ~GBAGetIF();
    //*interruptCheckFlagPtr = 0;
}

void GBACallIRQ(void)
{
    extern uint32_t irqsp;
    register uint32_t tmp;
    
    register uint32_t spsr;
    __asm__ volatile("mrs %0, spsr" : "=r"(spsr));
    if((spsr & 0x1f) == 0x12)
        for(;;);

    // CPSR mode should be already 0x12
    
    /*
    __asm__ volatile("mov r3, #0x4000000\n"
                     "mov lr, pc\n"
                     "ldr pc, [r3, #-4]"); // Pointer to first instruction of
                                           // user IRQ handler @ 0x4000000-4
    */
    __asm__ volatile("mov %0, sp" : "=r"(tmp));
    __asm__ volatile("ldr sp, %0" :: "m"(irqsp));
    __asm__ volatile("mov lr, pc\n"
                     " b 0x128" ::: "lr");
    __asm__ volatile("mov sp, %0" :: "r"(tmp));
    
    synchronizeInterruptFlags();
}

void GBASetIF(uint16_t flags)
{
    ((uint16_t*)wram1)[0x7FF8 >> 1] |= flags;
}

void GBAClearIF(uint16_t flags)
{
    ((uint16_t*)wram1)[0x7FF8 >> 1] &= ~flags;
}

uint16_t GBAGetIF(void)
{
    return ((uint16_t*)wram1)[0x7FF8 >> 1];
}

void GBASetInterruptFlags(uint16_t flags)
{
    volatile uint16_t *ptr = PERIPH16(0x202);
    uint16_t gbaIF = *ptr | flags;

    GBAClearIF(flags);
    *ptr = gbaIF;
}

void GBAClearInterruptFlags(uint16_t flags)
{
    volatile uint16_t *ptr = PERIPH16(0x202);
    uint16_t gbaIF = *ptr & ~flags;

    // Set Interrupt Check Flag @0x3007FF8
    GBASetIF(flags);
    *ptr = gbaIF;
}

