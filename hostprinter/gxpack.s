/* Licensed under the MIT license */
/* PACK -4096 of the GX driver: QuickDraw GX handles the Chooser */
	.section .text.entry,"ax"
	.globl _start
_start:
	bra.s   1f
	.short  0x4847              /* device ID */
	.ascii  "PACK"
	.short  -4096
	.short  0x0100              /* version */
	.long   0x08120800          /* left button, on/off, init, terminate */
globals:
	.space  40                  /* struct PackGlobals */
1:
	movem.l %d1-%d7/%a2-%a6,-(%sp)
	lea     globals(%pc),%a0
	move.l  %a0,-(%sp)
	pea     56(%sp)
	bsr.l   GXChooserC
	addq.l  #8,%sp
	movem.l (%sp)+,%d1-%d7/%a2-%a6
	move.l  (%sp)+,%a0
	lea     20(%sp),%sp
	move.w  %d0,(%sp)
	jmp     (%a0)
