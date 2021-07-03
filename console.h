#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

#include "framebuffer.h"

void ConsolePrint(uint32_t px, uint32_t py, const char *str);
void ConsolePrintHex(uint32_t px, uint32_t py, uint32_t x);

#endif

