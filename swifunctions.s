.section .text

.global SWI_VBlankIntrWait
SWI_VBlankIntrWait:
    mov r0, #1
    mov r1, #1
    ;@ Procedures continues with SWI_IntrWait
.global SWI_IntrWait
SWI_IntrWait:
    ldr r3, =0x03fffff8
    cmp r0, #0
    ;@ Clear old flags first, if r0 != 0
    strneh r1, [r3]
SWI_IntrWait_loop:
    ldrh r2, [r3]
    ands r2, r2, r1
    beq SWI_IntrWait_loop ;@ while(!(IF & r1))
    bx lr


