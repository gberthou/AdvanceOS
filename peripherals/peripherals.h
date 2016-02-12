#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <sys/types.h>

#include "../mmu.h"

#define PERIPH_SIZE 0x804

#define PERIPH32(x) (((volatile uint32_t*)periphdata) + ((x) >> 2))
#define PERIPH16(x) (((volatile uint16_t*)periphdata) + ((x) >> 1))

extern void *periphdata;

void PeripheralsInit(void);
void PeripheralsSetAccess(enum AccessRights accessRights);
void PeripheralsRefresh(uint32_t lastPeripheralAddress);

#endif

