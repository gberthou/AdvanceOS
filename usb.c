#include <sys/types.h>

#include "usb.h"
#include "irq.h"
#include "uspienv/interrupt.h"
#include "errlog.h"
#include "uspienv/timer.h"

#define IRQ_PENDING1  ((uint32_t*)0x2000B204)
#define IRQ_ENABLE1   ((uint32_t*)0x2000B210)
#define IRQ_DISABLE1  ((uint32_t*)0x2000B21C)

#define USB_IRQ_NUMBER 9

void USBCheckIRQ(void)
{
    if(*IRQ_PENDING1 & (1 << USB_IRQ_NUMBER))
    {
        RunUSBInterruptHandler();
        *IRQ_DISABLE1 |= (1 << USB_IRQ_NUMBER);
        usDelay(64);
        *IRQ_ENABLE1 |= (1 << USB_IRQ_NUMBER);
    }
}

void USBEnableIRQ(void)
{
    *IRQ_ENABLE1 |= (1 << USB_IRQ_NUMBER);
    IRQEnable();
}

