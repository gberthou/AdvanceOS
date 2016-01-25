/*
 * Pretty dumb mailbox system
 *   The MailboxReceive function should store the messages received in
 *   other channels instead of throwing them away. Since at this point I
 *   don't need the other channels, I keep it simple at the first place.
 */

#include "mailbox.h"

#define memoryBarrier() __asm__ volatile("mcr p15, 0, %0, c7, c10, 5" :: "r"(0))

#define MAIL_EMPTY (1 << 30)
#define MAIL_FULL  (1 << 31)

static volatile uint32_t * const MAIL0_READ   = (uint32_t*) 0x02000B880;
static volatile uint32_t * const MAIL0_STATUS = (uint32_t*) 0x02000B898;
static volatile uint32_t * const MAIL0_WRITE  = (uint32_t*) 0x02000B8A0;


static inline void waitForMailboxNonFull(void)
{
	register uint32_t status;
	do
	{
		memoryBarrier();
		status = *MAIL0_STATUS;
		memoryBarrier();
	} while(status & MAIL_FULL);
}

static inline void waitForMailboxNonEmpty(void)
{
	register uint32_t status;
	do
	{
		memoryBarrier();
		status = *MAIL0_STATUS;
		memoryBarrier();
	} while(status & MAIL_EMPTY);
}

static inline uint8_t readMail(uint32_t *data)
{
	register uint32_t mail;
	memoryBarrier();
	mail = *MAIL0_READ;
	memoryBarrier();
	
	*data = (mail & 0xFFFFFFF0);
	return mail & 0xF;
}

void MailboxSend(uint8_t channel, uint32_t data)
{
	register uint32_t w = (channel & 0xF) | (data & 0xFFFFFFF0);
	waitForMailboxNonFull();
	memoryBarrier();
	*MAIL0_WRITE = w;
	memoryBarrier();
}

uint32_t MailboxReceive(uint8_t channel)
{
	uint32_t data;

	do
	{
		waitForMailboxNonEmpty();
		memoryBarrier();

		// Here is the dumb part
	} while(readMail(&data) != channel);
	
	return data;
}

