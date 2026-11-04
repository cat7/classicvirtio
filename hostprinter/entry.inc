/* Licensed under the MIT license */

/* A Pascal entry point: saves the registers the caller keeps, passes C a
 * pointer to the parameters (the last one first), pops them and returns
 * the result. RESULT is 0, 1 (Boolean), 2 (short) or 4 bytes. */
	.macro PENTRY label, cfunc, params, result
\label:
	movem.l %d1-%d7/%a2-%a6,-(%sp)
	pea     52(%sp)
	bsr.l   \cfunc
	addq.l  #4,%sp
	movem.l (%sp)+,%d1-%d7/%a2-%a6
	move.l  (%sp)+,%a0
	lea     \params(%sp),%sp
	.if \result == 1
	move.b  %d0,(%sp)
	clr.b   1(%sp)
	.endif
	.if \result == 2
	move.w  %d0,(%sp)
	.endif
	.if \result == 4
	move.l  %d0,(%sp)
	.endif
	jmp     (%a0)
	.endm
