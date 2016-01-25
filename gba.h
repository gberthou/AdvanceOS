#ifndef GBA_H
#define GBA_H

#define GBA_BIOS_SIZE 0x4000
#define GBA_ROM_SIZE 0x2000000

void GBALoadComponents(void);
void GBARun(void);
void GBACallIRQ(void);

#endif

