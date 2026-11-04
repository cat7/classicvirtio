/* Licensed under the MIT license */
/* PACK -4096: the Chooser's device package */
	.include "entry.inc"
	.section .text.entry,"ax"
	.globl _start
_start:
	bra.s   1f
	.short  0x4850              /* device ID */
	.ascii  "PACK"
	.short  -4096
	.short  0x0100              /* version */
	.long   0x0000F800          /* fill list, get/select/deselect, terminate */
	PENTRY  1, ChooserC, 20, 2
