/* Licensed under the MIT license */
/* 'pdvr' 0 of the GX driver: the offsets in the 'over' resources point
 * at these entries. QuickDraw GX calls them as C functions. */
	.section .text.entry,"ax"
	.globl _start
_start:
	.long   0
	bra.w   HPOpenConnection    /* 4 */
	bra.w   HPCloseConnection   /* 8 */
	bra.w   HPStartSendPage     /* 12 */
	bra.w   HPFinishSendPage    /* 16 */
	bra.w   HPWriteData         /* 20 */
	bra.w   HPRasterDataIn      /* 24 */
	bra.w   HPDefaultPrinter    /* 28 */
	bra.w   HPSetupImageData    /* 32 */
	bra.w   HPDefaultDesktopPrinter /* 36 */
