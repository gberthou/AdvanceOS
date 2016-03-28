#include "timer.h"
#include "peripherals/peripherals.h"
#include "peripherals/lcd.h"
#include "peripherals/timer.h"
#include "uspienv/timer.h"
#include "errlog.h"

#define TMR_CHI (TMR_CS + 2)
#define TMR_C   (TMR_CS + 3)
#define TMR_C0  (TMR_CS + 3)
#define TMR_C1  (TMR_CS + 4)
#define TMR_C2  (TMR_CS + 5)
#define TMR_C3  (TMR_CS + 6)

#define IRQ_ENABLE1       ((uint32_t*)0x2000B210)
#define IRQ_ENABLE2       ((uint32_t*)0x2000B214)
#define IRQ_ENABLE_BASIC  ((uint32_t*)0x2000B218)
#define IRQ_DISABLE1      ((uint32_t*)0x2000B21C)
#define IRQ_DISABLE2      ((uint32_t*)0x2000B220)
#define IRQ_DISABLE_BASIC ((uint32_t*)0x2000B224)

#define TIMER_LCD_CHAN 1
//#define TIMER_GBA_CHAN 1
#define TIMER_USB_CHAN 3

#define TIMER_LCD_MASK (1 << TIMER_LCD_CHAN)
//#define TIMER_GBA_MASK (1 << TIMER_GBA_CHAN)
#define TIMER_USB_MASK (1 << TIMER_USB_CHAN)

/* Timer mapping
 * Timer0: Reserved by GPU, cf.
 * https://github.com/xinu-os/xinu/blob/master/system/platforms/arm-rpi/timer.c
 * Timer1: LCD Timer
 * Timer2: Reserved by GPU
 * Timer3: USB Timer
 */

void TimerInit(void)
{
    // Clear pending flags
    *TMR_CS = TIMER_LCD_MASK | TIMER_USB_MASK;
    
    *IRQ_DISABLE1 = TIMER_LCD_MASK | TIMER_USB_MASK; 
}

void TimerEnableLCD(void)
{
    LCDInitClock(*TMR_CLO);
    TMR_C[TIMER_LCD_CHAN] = *TMR_CLO + CLOCK_LCD;
    *TMR_CS = TIMER_LCD_MASK;
    *IRQ_ENABLE1 = TIMER_LCD_MASK;

    TimerDisableUSB();
}

void TimerEnableUSB(uint32_t deadline)
{
    TMR_C[TIMER_USB_CHAN] = deadline;
    *TMR_CS = TIMER_USB_MASK;
    *IRQ_ENABLE1 = TIMER_USB_MASK;
}

void TimerDisableUSB(void)
{
    *IRQ_DISABLE1 = TIMER_USB_MASK;
}

void TimerCheckIRQ(void)
{
    uint32_t csValue = *TMR_CS;
    uint32_t newCsValue = 0;

    if(csValue & TIMER_LCD_MASK)
    {
        uint32_t nextdeadline;
        LCDOnTick(*TMR_CLO);

        nextdeadline = *TMR_CLO + CLOCK_LCD;
        TMR_C[TIMER_LCD_CHAN] = nextdeadline;
        
        RunFirstUSBTimerHandler();
        newCsValue |= TIMER_LCD_MASK;
    }
    
    /*
    if(csValue & TIMER_GBA_MASK)
    {
        uint32_t ticks = TMR_C[TIMER_GBA_CHAN];
        uint32_t currentTicks;

        //TimerOnTick();
        
        ticks += CLOCK_TIMER;
        currentTicks = *TMR_CLO;
        if(ticks <= currentTicks)
            ticks = currentTicks + CLOCK_TIMER;
        TMR_C[TIMER_GBA_CHAN] = ticks;
    }
    */

    if(csValue & TIMER_USB_MASK)
    {
        RunFirstUSBTimerHandler();
        newCsValue |= TIMER_USB_MASK;
    }

    // Clear flags
    *TMR_CS = newCsValue;
}

