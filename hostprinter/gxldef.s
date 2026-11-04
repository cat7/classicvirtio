/* Licensed under the MIT license */
/* LDEF -4096 of the GX driver */
	.include "entry.inc"
	.section .text.entry,"ax"
	.globl _start
_start:
	PENTRY  1, GXLdefC, 20, 0
