#include "time.h"
#include "peripherals/peripherals.h"
#include "peripherals/lcd.h"
#include "peripherals/timer.h"

#define TMR_BASE 0x20003000

#define TMR_CS  ((volatile uint32_t*)TMR_BASE)
#define TMR_CLO (TMR_CS + 1)
#define TMR_CHI (TMR_CS + 2)
#define TMR_C0  (TMR_CS + 3)
#define TMR_C1  (TMR_CS + 4)
#define TMR_C2  (TMR_CS + 5)
#define TMR_C3  (TMR_CS + 6)

#define IRQ_ENABLE1 ((uint32_t*)0x2000B210)

//#define CLOCK_FREQ 700000000
#define CLOCK_FREQ 7000000

#define CLOCK_LCD (CLOCK_FREQ/13620)
#define CLOCK_EPSILON 1000

#define CLOCK_TIMER CLOCK_FREQ

void TimerLCDInit(void)
{
	*TMR_C1 = *TMR_CLO + CLOCK_LCD;
	*TMR_C2 = *TMR_CLO + CLOCK_TIMER;
	*TMR_CS |= 6;
	*IRQ_ENABLE1 |= 6; // Enable ARM Timer IRQ
}

void TimerCheckIRQ(void)
{
	register uint32_t csValue = *TMR_CS;
	if(csValue & 2) // LCD timer
	{
        uint32_t ticks = *TMR_C1;
        uint32_t currentTicks;
		LCDOnTick();

		*TMR_CS = csValue | 2; // Clear interrupt flag
		ticks += CLOCK_LCD;
        currentTicks = *TMR_CLO;
		if(ticks < currentTicks)
			ticks = currentTicks + CLOCK_LCD;
        *TMR_C1 = ticks;
	}
	
	if(csValue & 4) // GBA timer
	{
        uint32_t ticks = *TMR_C2;
        uint32_t currentTicks;
		//TimerOnTick();

		*TMR_CS = csValue | 4; // Clear interrupt flag
		ticks += CLOCK_TIMER;
        currentTicks = *TMR_CLO;
		if(ticks <= currentTicks)
			ticks = currentTicks + CLOCK_TIMER;
        *TMR_C2 = ticks;
	}
}

