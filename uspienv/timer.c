#include <sys/types.h>
#include <uspios.h>
#include <uspi/string.h>

#include "../timer.h"
#include "../errlog.h"

// TIME_RATIO: SystemTimerFreq / HZ (macro defined in uspios.h)
#define TIME_RATIO (1000000 / HZ)

#define TIMERBLOCK_COUNT 64

struct TimerBlock
{
    uint32_t deadline;
    uint8_t overflowdeadline;
    TKernelTimerHandler *handler;
    void *param;
    void *context;
    
    struct TimerBlock *next;
};

static struct TimerBlock timerblocks[TIMERBLOCK_COUNT];
static uint8_t timerblocksAvailable[TIMERBLOCK_COUNT >> 3];

// Chained list of queued timer blocks, ordered by deadline
// (list head = earliest deadline)
static struct TimerBlock *queuedBlocks;

void DelayLoop(unsigned int nCount);

void USPiEnvTimerInit(void)
{
    unsigned int i;
    for(i = 0; i < (TIMERBLOCK_COUNT >> 3); ++i)
        timerblocksAvailable[i] = 0;

    queuedBlocks = 0;
}

static struct TimerBlock *allocateTimerBlock(void)
{
    unsigned int i;
    for(i = 0; i < (TIMERBLOCK_COUNT >> 3); ++i)
    {
        if(timerblocksAvailable[i] != 0xFF)
        {
            unsigned int j;
            for(j = 0; j < 8 && (timerblocksAvailable[i] & (1 << j)); ++j);
            // At this point, 0 <= j <= 7
            timerblocksAvailable[i] |= (1 << j);
            return timerblocks + (i << 3) + j;
        }
    }
    return 0;
}

static void freeTimerBlock(const struct TimerBlock *block)
{
    size_t index = block - timerblocks;
    timerblocksAvailable[index >> 3] &= ~(1 << (index & 0x7));
}

static int compareTimerBlocks(const struct TimerBlock *a,
                              const struct TimerBlock *b)
{
    if(a->overflowdeadline == b->overflowdeadline)
    {
        if(a->deadline < b->deadline)
            return -1;
        if(a->deadline == b->deadline)
            return 0;
        return 1;
    }
    if(a->overflowdeadline) // b's deadline does not overflow
        return 1; // Then a's deadline is bigger
    return -1; // a's deadline is smaller
}

static void insertQueuedBlock(struct TimerBlock *block)
{
    if(!queuedBlocks) // Queued blocks list was empty
    {
        block->next = 0; // End of list
        queuedBlocks = block;
    }
    else
    {
        struct TimerBlock *it;
        struct TimerBlock *prec = 0;
        for(it = queuedBlocks; it; prec = it, it = it->next)
        {
            if(compareTimerBlocks(block, it) <= 0)
            {
                block->next = it;
                if(prec)
                    prec->next = block;
                else
                    queuedBlocks = block;
                return;
            }
        }
        
        // Parameter block has either the smallest or the biggest deadline
        if(prec) // Parameter block has the biggest deadline
        {
            block->next = 0;
            prec->next = block; // Insert block at the end of the list
        }
        else // Parameter block has the smallest deadline
        {
            block->next = queuedBlocks;
            queuedBlocks = block; // Insert block at the beginning of the
                                  // list
        }
    }
}

static void removeFirstQueuedBlock(void)
{
    if(queuedBlocks)
        queuedBlocks = queuedBlocks->next;
}

void usDelay(unsigned int us)
{
    /*
    uint32_t clo = *TMR_CLO;
    uint32_t next = clo + us;

    if(next < clo)
        while(*TMR_CLO > next);
    while(*TMR_CLO < next);
    */
    /*
    uint32_t next = *TMR_CLO+us;
    while(*TMR_CLO < next);
    */

    DelayLoop(12 * us);
}

void MsDelay(unsigned int ms)
{
    /*
    uint32_t clo = *TMR_CLO;
    uint32_t next = clo + ms * 1000;

    if(next < clo)
        while(*TMR_CLO > next);
    while(*TMR_CLO < next);
    */
   
    /* 
    uint32_t next = *TMR_CLO+1000*ms;
    while(*TMR_CLO < next);
    */

    DelayLoop(12500 * ms);
}

static void updateUSBTimer(void)
{
    if(queuedBlocks)
        TimerEnableUSB(queuedBlocks->deadline);
    else
        TimerDisableUSB();
}

// Here, one delay unit = 1/(100 Hz)
unsigned int StartKernelTimer(unsigned int nDelayUnits,
                              TKernelTimerHandler *handler,
                              void *param, void *context)
{
    uint32_t ticks = *TMR_CLO;
    struct TimerBlock *block = allocateTimerBlock();

    /*
    TString s;
    String(&s);
    StringFormat(&s, "delay: %d", nDelayUnits);
    LogWrite("gamepad", LOG_ERROR, s.m_pBuffer);
*/

    if(!block) // There is no available block
    {
        ErrorDisplayMessage("StartKernelTimer: no available block", 1);
    }

    if(nDelayUnits < 1000)
        nDelayUnits = 1000;

    block->deadline = ticks + nDelayUnits;
    block->overflowdeadline = (block->deadline < ticks);
    block->handler = handler;
    block->param = param;
    block->context = context;
    
    insertQueuedBlock(block);

    updateUSBTimer();

    return block - timerblocks + 1;
}

void CancelKernelTimer(unsigned int hTimer)
{
    struct TimerBlock *it;
    struct TimerBlock *prec = 0;
    for(it = queuedBlocks; it; prec = it, it = it->next)
    {
        if((int)hTimer == it - timerblocks)
        {
            if(prec)
                prec->next = it->next;
            else // Block was the head of list
                queuedBlocks = it->next;
            freeTimerBlock(it);
            return;
        }
    }
    updateUSBTimer();
}

void RunFirstUSBTimerHandler(void)
{
    static uint32_t prevTicks = 0;
    uint32_t ticks = *TMR_CLO;

    if(ticks < prevTicks) // timer overflow
    {
        struct TimerBlock *it;
        for(it = queuedBlocks; it; it = it->next)
            it->overflowdeadline = 0;
    }
    prevTicks = ticks;

    while(queuedBlocks && !queuedBlocks->overflowdeadline
                       && queuedBlocks->deadline <= *TMR_CLO) // Execute all the
                                                              // late jobs
    {
        struct TimerBlock *block = queuedBlocks;

        // TODO: Check whether +1 is correct here
        (*(block->handler)) (block - timerblocks + 1, block->param,
                             block->context);

        removeFirstQueuedBlock();
        freeTimerBlock(block);
    }
    
    updateUSBTimer();
}

