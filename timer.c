#include "timer.h"
#include "peripherals/peripherals.h"
#include "peripherals/lcd.h"
#include "peripherals/timer.h"

#define TMR_CHI (TMR_CS + 2)
#define TMR_C0  (TMR_CS + 3)
#define TMR_C1  (TMR_CS + 4)
#define TMR_C2  (TMR_CS + 5)
#define TMR_C3  (TMR_CS + 6)

#define IRQ_ENABLE1 ((uint32_t*)0x2000B210)

void TimerInit(void)
{
    *TMR_C1 = *TMR_CLO + CLOCK_LCD;
    *TMR_C2 = *TMR_CLO + CLOCK_TIMER;
    *TMR_CS |= 6;

    LCDInitClock(*TMR_CLO);
    
    *IRQ_ENABLE1 |= 6; // Enable ARM Timer IRQ
}

void TimerCheckIRQ(void)
{
    register uint32_t csValue = *TMR_CS;
    if(csValue & 2) // LCD timer
    {
        LCDOnTick(*TMR_CLO);

        *TMR_CS = csValue | 2; // Clear interrupt flag
        *TMR_C1 = *TMR_CLO + CLOCK_LCD;
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

