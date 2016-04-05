.section .text
.global start
start:
    ;@ Set interrupt vector base address to 0xFFFF0000
    ;@mrc p15, 0, r0, c1, c0, 0
    ;@orr r0, #0x2000 ;@ r0 |= (1 << 13) V bit
    ;@mcr p15, 0, r0, c1, c0, 0
    
    ;@ Disable IRQ
    ;@mov r0, #0xC0
    ;@msr cpsr_c, r0

    ;@ Init IRQ stack
    cps #0x12
    ldr sp, irqStack

    ;@ Init Abort stack
    cps #0x17
    ldr sp, abortStack

    ;@ Init User stack
    cps #0x1f
    ldr sp, usrStack

    ;@ Init SVC stack
    cps #0x13    
    ldr sp, svcStack

    bl main ;@ Squadala!
    b start

.global __stack_begin
__stack_begin:
abortStack: .word 0x00006700
irqStack:   .word 0x00006800
svcStack:   .word 0x00007000
usrStack:   .word 0x00007800

.global PrefetchHandler
PrefetchHandler:
    mrs r0, spsr
PH1:
    b PH1

.global UndefinedHandler
UndefinedHandler: b UndefinedHandler

.global UnusedHandler
UnusedHandler: b UnusedHandler

.global SwiHandler
SwiHandler: b 0x8

.global IRQHandler
IRQHandler:
    push {r0}
    mrs r0, spsr
    and r0, r0, #0x1f
    cmp r0, #0x12
IRQHandlerHold:
    beq IRQHandlerHold

    ldr r0, =irqsp
    str sp, [r0]
    mov r0, sp
    ldr sp, irqStack
    push {r0-r12, lr}
    ldr r0, =irqlr
    str lr, [r0]
    bl USBCheckIRQ
    bl TimerCheckIRQ
    pop {r0-r12, lr}
    mov sp, r0
    pop {r0}
    subs pc, lr, #4

.global FIQHandler
FIQHandler: b FIQHandler

.global irqlr
irqlr: .word 42

.global irqsp
irqsp: .word 43

