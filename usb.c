#include <stdint.h>

#include <uspi/string.h>

#include "usb.h"
#include "irq.h"
#include "uspienv/interrupt.h"
#include "errlog.h"
#include "uspienv/timer.h"
#include "console.h"

#define IRQ_PENDING1  ((uint32_t*)0x2000B204)
#define IRQ_ENABLE1   ((uint32_t*)0x2000B210)
#define IRQ_DISABLE1  ((uint32_t*)0x2000B21C)

#define USB_IRQ_NUMBER 9

void USBCheckIRQ(void)
{
#ifndef NO_USB
    if(*IRQ_PENDING1 & (1 << USB_IRQ_NUMBER))
    {
        RunUSBInterruptHandler();
        *IRQ_PENDING1 = (1 << USB_IRQ_NUMBER);
    }
#endif
}

void USBEnableIRQ(void)
{
#ifndef NO_USB
    *IRQ_ENABLE1 = (1 << USB_IRQ_NUMBER);
    IRQEnable();
#endif
}

void USBDisableIRQ(void)
{
#ifndef NO_USB
    *IRQ_DISABLE1 = (1 << USB_IRQ_NUMBER);
#endif
}

