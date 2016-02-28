#ifndef ENV_TIMER_H
#define ENV_TIMER_H

void USPiEnvTimerInit(void);
void RunFirstUSBTimerHandler(void);

void usDelay(unsigned int us);
void MsDelay(unsigned int ms);

#endif

