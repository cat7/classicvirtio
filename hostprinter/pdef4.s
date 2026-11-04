/* Licensed under the MIT license */
/* PDEF 4: the jump table, then the entry points */
	.include "entry.inc"
	.section .text.entry,"ax"
	.globl _start
_start:
	bra.w   1f
	bra.w   2f
	bra.w   3f
	bra.w   4f
	bra.w   5f
	bra.w   6f
	bra.w   7f
	bra.w   8f
	PENTRY  1, PrintDefaultC, 4, 0
	PENTRY  2, PrStlDialogC, 4, 1
	PENTRY  3, PrJobDialogC, 4, 1
	PENTRY  4, PrStlInitC, 4, 4
	PENTRY  5, PrJobInitC, 4, 4
	PENTRY  6, PrDlgMainC, 8, 1
	PENTRY  7, PrValidateC, 4, 1
	PENTRY  8, PrJobMergeC, 8, 0
