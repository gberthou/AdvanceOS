#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <stdint.h>

#include "../mmu.h"

#define PERIPH_BASE 0x04000000
#define PERIPH_SIZE 0x804

#define PERIPH32(x) (((volatile uint32_t*)periphdata) + ((x) >> 2))
#define PERIPH16(x) (((volatile uint16_t*)periphdata) + ((x) >> 1))
#define PERIPH8(x) (((volatile uint8_t*)periphdata) + (x))

extern void *periphdata;

void PeripheralsInit(void);
void PeripheralsRefresh(uint32_t address);

#endif

