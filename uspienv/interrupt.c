#include <uspios.h>

#include "../usb.h"
#include "../errlog.h"

#define USB_IRQ_NUMBER 9

struct IRQHandlerContainer
{
    TInterruptHandler *handler;
    void *param;
};

static struct IRQHandlerContainer handlerContainer;

void USPiEnvInterruptInit(void)
{
    handlerContainer.handler = 0;
} 

void ConnectInterrupt(unsigned int nIRQ,
                      TInterruptHandler *handler, void *param)
{
    if(nIRQ == USB_IRQ_NUMBER)
    {
        handlerContainer.handler = handler;
        handlerContainer.param = param;
        USBEnableIRQ();
    }
    else // Denied
    {
        ErrorDisplayMessage("ConnectInterrupt: bad nIRQ");
    }
}

void RunUSBInterruptHandler(void)
{
    if(handlerContainer.handler)
    {
        (*handlerContainer.handler) (handlerContainer.param);
    }
    else
        ErrorDisplayMessage("Nope");
}

