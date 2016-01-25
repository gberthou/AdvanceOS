#ifndef IRQ_H
#define IRQ_H

#define IRQEnable() __asm__ volatile("push {r0}\n"\
									 "mrs r0, cpsr\n"\
									 "bic r0, r0, #0x80\n"\
									 "msr cpsr, r0\n"\
									 "pop {r0}\n")

#define IRQDisable() __asm__ volatile("push {r0}\n"\
									  "mrs r0, cpsr\n"\
									  "orr r0, r0, #0x80\n"\
									  "msr cpsr, r0\n"\
									  "pop {r0}\n")

#endif

