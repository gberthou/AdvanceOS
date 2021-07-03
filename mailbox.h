#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>

void MailboxSend(uint8_t channel, uint32_t data);
uint32_t MailboxReceive(uint8_t channel);

#endif

