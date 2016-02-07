#ifndef GBA_H
#define GBA_H

#include <sys/types.h>

#define GBA_BIOS_SIZE 0x4000
#define GBA_ROM_SIZE 0x2000000

extern void *wram1;

void GBALoadComponents(void);
void GBARun(void);
void GBACallIRQ(void);

void GBASetInterruptFlags(uint16_t flags);
void GBAClearInterruptFlags(uint16_t flags);

#endif

