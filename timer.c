#include "timer.h"
#include "peripherals/peripherals.h"
#include "peripherals/lcd.h"
#include "peripherals/timer.h"
#include "uspienv/timer.h"

#define TMR_CHI (TMR_CS + 2)
#define TMR_C   (TMR_CS + 3)
#define TMR_C0  (TMR_CS + 3)
#define TMR_C1  (TMR_CS + 4)
#define TMR_C2  (TMR_CS + 5)
#define TMR_C3  (TMR_CS + 6)

#define IRQ_ENABLE1  ((uint32_t*)0x2000B210)
#define IRQ_DISABLE1 ((uint32_t*)0x2000B21C)

#define TIMER_LCD_CHAN 0
#define TIMER_GBA_CHAN 1
#define TIMER_USB_CHAN 2

#define TIMER_LCD_MASK (1 << TIMER_LCD_CHAN)
#define TIMER_GBA_MASK (1 << TIMER_GBA_CHAN)
#define TIMER_USB_MASK (1 << TIMER_USB_CHAN)

/* Timer mapping
 * Timer0: LCD
 * Timer1: GBA Timers
 * Timer2: USB
 * Timer3: Free
 */

void TimerInit(void)
{
    TMR_C[TIMER_LCD_CHAN] = *TMR_CLO + CLOCK_LCD;
    TMR_C[TIMER_GBA_CHAN] = *TMR_CLO + CLOCK_TIMER;
    *TMR_CS |= TIMER_LCD_MASK /*| TIMER_GBA_MASK | TIMER_USB_MASK*/;

    LCDInitClock(*TMR_CLO);
    
    *IRQ_ENABLE1 |= TIMER_LCD_MASK /*| TIMER_GBA_MASK*/;
}

void TimerEnableUSB(uint32_t deadline)
{
    TMR_C[TIMER_USB_CHAN] = deadline;
    *IRQ_ENABLE1 |= TIMER_USB_MASK;
}

void TimerDisableUSB(void)
{
    *IRQ_DISABLE1 |= TIMER_USB_MASK;
}
void TimerCheckIRQ(void)
{
    register uint32_t csValue = *TMR_CS;
    uint32_t newcsValue = 0;

    if(csValue & TIMER_LCD_MASK)
    {
        uint32_t nextdeadline;
        newcsValue |= TIMER_LCD_MASK; // Clear interrupt flag
        LCDOnTick(*TMR_CLO);

        nextdeadline = *TMR_CLO + CLOCK_LCD;
        TMR_C[TIMER_LCD_CHAN] = nextdeadline;
        *TMR_CS = newcsValue;
    }
    
    /*
    if(csValue & TIMER_GBA_MASK)
    {
        uint32_t ticks = TMR_C[TIMER_GBA_CHAN];
        uint32_t currentTicks;

        newcsValue |= TIMER_GBA_MASK; // Clear interrupt flag
        
        //TimerOnTick();
        
        ticks += CLOCK_TIMER;
        currentTicks = *TMR_CLO;
        if(ticks <= currentTicks)
            ticks = currentTicks + CLOCK_TIMER;
        TMR_C[TIMER_GBA_CHAN] = ticks;
    }

    if(csValue & TIMER_USB_MASK)
    {
        newcsValue |= TIMER_USB_MASK; // Clear interrupt flag
        RunFirstUSBTimerHandler();
    }
    */

    //*TMR_CS = newcsValue;
}

