#ifndef GBA_H
#define GBA_H

#include <stdint.h>

#define GBA_BIOS_SIZE 0x4000
#define GBA_ROM_SIZE 0x2000000

void GBALoadComponents(void);
void GBARun(void);
void GBACallIRQ(void);

void GBASetIF(uint16_t flags);
void GBAClearIF(uint16_t flags);
uint16_t GBAGetIF(void);

void GBASetInterruptFlags(uint16_t flags);
void GBAClearInterruptFlags(uint16_t flags);

#endif

