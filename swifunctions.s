.section .text
.global SWI_IntrWait
SWI_IntrWait:
    ldr r3, =0x03fffff8
    ;@ldr r3, [r3]
    ;@ Offset 0x202 = 0x200 + 2
    ;@add r3, r3, #0x200
    cmp r0, #0
    beq SWI_IntrWait_loop
    ;@ Clear old flags first
    strh r1, [r3]
SWI_IntrWait_loop:
    ldrh r2, [r3]
    ands r2, r2, r1
    beq SWI_IntrWait_loop ;@ while(!(IF & r1))
    bx lr

.global SWI_VBlankIntrWait
SWI_VBlankIntrWait:
    mov r0, #1
    mov r1, #1
    ldr pc, =SWI_IntrWait


