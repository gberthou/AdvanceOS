#ifndef TIMER_H
#define TIMER_H

#include <sys/types.h>

//#define CLOCK_FREQ 700000000
#define CLOCK_FREQ 7000000
#define CLOCK_LCD (CLOCK_FREQ/13620)
#define CLOCK_TIMER CLOCK_FREQ

void TimerInit(void);
void TimerCheckIRQ(void);
uint32_t TimerGetTicks(void);

#endif

