/* Licensed under the MIT license */
/* PDEF 0 and 1: the jump table, then the entry points */
	.include "entry.inc"
	.section .text.entry,"ax"
	.globl _start
_start:
	bra.w   1f
	bra.w   2f
	bra.w   3f
	bra.w   4f
	PENTRY  1, PrOpenDocC, 12, 4
	PENTRY  2, PrCloseDocC, 4, 0
	PENTRY  3, PrOpenPageC, 8, 0
	PENTRY  4, PrClosePageC, 4, 0
