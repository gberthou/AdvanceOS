#ifndef TIMER_H
#define TIMER_H

#include <sys/types.h>

//#define CLOCK_FREQ 700000000
#define CLOCK_FREQ 7000000
#define CLOCK_LCD (CLOCK_FREQ/13620)
#define CLOCK_TIMER CLOCK_FREQ

#define TMR_BASE 0x20003000
#define TMR_CS  ((volatile uint32_t*)TMR_BASE)
#define TMR_CLO (TMR_CS + 1)

void TimerInit(void);
void TimerCheckIRQ(void);

#define TimerGetTicks() (*TMR_CLO)

#endif

