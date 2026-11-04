/* Licensed under the MIT license */
/* DRVR -8192: the .Print driver the Printing Manager opens. Everything
 * goes through the PDEFs, so it only has to answer. */
	.section .text.entry,"ax"
	.globl _start
_start:
	.short  0x4f00              /* needs lock, read, write, control, status */
	.short  0, 0, 0
	.short  open-_start
	.short  prime-_start
	.short  ctl-_start
	.short  stat-_start
	.short  close-_start
	.byte   6
	.ascii  ".Print"
	.align  2
open:
close:
	moveq   #0,%d0
	rts
prime:
ctl:
stat:
	moveq   #0,%d0
	btst    #1,6(%a0)           /* ioTrap: immediate (bit 9 of the word) */
	bne.s   1f
	move.l  0x08fc,-(%sp)       /* JIODone */
1:
	rts
