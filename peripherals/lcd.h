#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void LCDInitClock(uint32_t clock);
void LCDRefresh(void);
void LCDOnTick(uint32_t clock);
void LCDUpdateScreen(void);

#endif

