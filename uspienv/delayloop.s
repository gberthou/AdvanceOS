/*
 * delayloop.S
 */

	.text

	.align	3

	.globl	DelayLoop
DelayLoop:
	subs	r0, r0, #1
	bhi	DelayLoop
	bx lr

/* End */
