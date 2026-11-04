/* Licensed under the MIT license */
/* PDEF 7: PrGeneral */
	.include "entry.inc"
	.section .text.entry,"ax"
	.globl _start
_start:
	PENTRY  1, PrGeneralC, 4, 0
