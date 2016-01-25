#include "timer.h"
#include "peripherals.h"

#define P_TIMER_BASE 0x04000100

struct TimerInfo
{
	unsigned int started;
	unsigned int irqEnabled;
	uint16_t counter;
	uint32_t tickCounter;
	unsigned int countup;
	unsigned int prescaler;
};

static struct TimerInfo timerChannels[4];

static void TimerRefreshChannel(unsigned int channel)
{
	const uint16_t *ptr = (uint16_t*) (P_TIMER_BASE + 0x4 * channel);
	uint16_t control = ptr[1];

	if(control & (1 << 7)) // Start timer
	{
		if(!timerChannels[channel].started) // Not running before this point
		{
			timerChannels[channel].counter = ptr[0];
			timerChannels[channel].tickCounter = 0;
			if(control & (1 << 2)) // Count-up
				timerChannels[channel].countup = 1;
			else
			{
				timerChannels[channel].countup = 0;
				timerChannels[channel].prescaler = control & 0x3;
			}
			timerChannels[channel].irqEnabled = control & (1 << 6);
		}
		timerChannels[channel].started = 1;
	}
	else // Stop timer
		timerChannels[channel].started = 0;
}

void TimerRefresh(void)
{
	unsigned int i;
	for(i = 0; i < 4; ++i)
		TimerRefreshChannel(i);
}

void TimerOnTick(void)
{
	const uint32_t PRESCALER_SHIFTS[] = {
		0,
		6,
		8,
		10
	};

	unsigned int i;
	for(i = 0; i < 4; ++i)
	{
		if(timerChannels[i].started)
		{
			++timerChannels[i].tickCounter;
			if(!timerChannels[i].countup)
			{
				uint32_t prescaler = timerChannels[i].prescaler;
				uint32_t prevCounter = timerChannels[i].counter;
				timerChannels[i].counter += timerChannels[i].tickCounter >> PRESCALER_SHIFTS[prescaler];
				timerChannels[i].tickCounter &= (1 << PRESCALER_SHIFTS[prescaler]) - 1;
				
				if(i < 3 && timerChannels[i].counter < prevCounter) // Overflow
				{
					if(timerChannels[i+1].started && timerChannels[i+1].countup)
					{
						++timerChannels[i+1].counter;
						// Warning: this timer might overflow too and this case
						// is not yet handled
					}
				}
				
				*(uint16_t*)(periphdata + (0x100>>2) + i) = timerChannels[i].counter;
			}
		}
	}
}

