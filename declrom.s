/* Copyright (c) 2023 Elliot Nunn */
/* Licensed under the MIT license */

	/* random defines and macros that we would normally get from MPW */
	.macro OSLstEntry Id,Offset
	.long (\Id<<24)+((\Offset-.)&0xFFFFFF)
	.endm
	.macro DatLstEntry Id,Data
	.long (\Id<<24)+(\Data&0xFFFFFF)
	.endm
	.set sCPU_68020, 2
	.set endOfList, 255
	.set sRsrcType, 1
	.set sRsrcName, 2
	.set sRsrcIcon, 3
	.set sRsrcDrvrDir, 4
	.set sRsrcLoadRec, 5
	.set sRsrcBootRec, 6
	.set sRsrcFlags, 7
	.set sRsrcHWDevId, 8
	.set minorBase, 10
	.set minorLength, 11
	.set majorBase, 12
	.set majorLength, 13
	.set sRsrcCicn, 15
	.set StartupZoneSize, 0x60000 /* see StubDriver */
	.set boardId, 32
	.set primaryInit, 34
	.set vendorInfo, 36
	.set sGammaDir, 64
	.set vendorId, 1
	.set revLevel, 3
	.set partNum, 4
	.set mVidParams, 1
	.set mPageCnt, 3
	.set mDevType, 4
	.set oneBitMode, 128
	.set twoBitMode, 129
	.set fourBitMode, 130
	.set eightBitMode, 131
	.set sixteenBitMode, 132
	.set thirtyTwoBitMode, 133
	.set catBoard, 1
	.set catDisplay, 3
	.set typeBoard, 0
	.set typeVideo, 1
	.set typeVideo, 1
	.set typeDesk, 2
	.set drSwMacCPU, 0
	.set catCPU, 10
	.set catNetwork, 4
	.set typeEthernet, 1
	.set drSwApple, 1
	.set picOffset, 128 /* board sResource: where the goldfish PIC is, if not at +0 */
	/* SpBlock; parameter block for Slot Manager routines */
	.struct 0
spResult: .space 4
spsPointer: .space 4
spSize: .space 4
spOffsetData: .space 4
spIOFileName: .space 4
spsExecPBlk: .space 4
spParamData: .space 4
spMisc: .space 4
spReserved: .space 4
spIOReserved: .space 2
spRefNum: .space 2
spCategory: .space 2
spCType: .space 2
spDrvrSW: .space 2
spDrvrHW: .space 2
spTBMask: .space 1
spSlot: .space 1
spID: .space 1
spExtDev: .space 1
spHwDev: .space 1
spByteLanes: .space 1
spFlags: .space 1
spKey: .space 1
spBlock.size:
	/* SEBlock; parameter block for SExec blocks (like PrimaryInit) */
	.struct 0
seSlot: .space 1
sesRsrcId: .space 1
seStatus: .space 2
seFlags: .space 1
seFiller0: .space 1
seFiller1: .space 1
seFiller2: .space 1
seResult: .space 4
seIOFileName: .space 4
seDevice: .space 1
sePartition: .space 1
seOSType: .space 1
seReserved: .space 1
seRefNum: .space 1
seNumDevices: .space 1
seBootState: .space 1
filler: .space 1
SEBlock.size:
	.set JIODone, 0x08FC
	.set JVBLTask, 0x0D28
	.set ioTrap, 6
	.set ioResult, 16
	.set noQueueBit, 9
	.set dCtlSlot, 40

/* The SlotMgr helpfully copies "slotExec" code blocks into scratch RAM to run.
 * Jump back to ROM so our code can take valid function pointers from itself.
 */
	.macro  SlotExecROMStub end
	.long   \end-.                     /* my length: the whole block */
	.long   0x02010000                 /* struct version / 68000 / reserved */
	.long   4                          /* offset to code (i.e. right here) */
	/* In 24-bit mode (always on the Mac II family at this point, or with
	 * 32-bit addressing off) the ROM, at the top of the slot, is out of
	 * reach: run the Slot Manager's RAM copy of this block instead. The
	 * code is position-independent and keeps no pointers to itself. */
	tst.b   0x0cb2                     /* MMU32Bit */
	beq.s   20f
	clr.l   %d0
	move.b  (%a0),%d0                  /* get seSlot */
	ror.l   #8,%d0                     /* move to 0F000000 position */
	add.l   #20f-ROMEND+0xf1000000,%d0 /* tricky delta */
	move.l  %d0,%a1
	jmp     (%a1)                      /* to the ROM copy of the next instruction */
	20:
	.endm


/* Put A0 on the stack for C code like this: void func(void *a0);
 */
	.macro  SlotExecCShim
	movem.l %d0-%d7/%a0-%a6,-(%sp)
	move.l  %a0,-(%sp)
	bsr.s   30f
	addq    #4,%sp
	movem.l (%sp)+,%d0-%d7/%a0-%a6
	clr.l   %d0
	rts
	30:
	.endm


	.macro  StubDriver myname, elf, elfend, firstid, vtype
	.long   69f-.
60:
	.short  0x4f00              /* needtime + all calls enabled */
	.short  0                   /* drvrDelay */
	.short  0                   /* drvrEMask */
	.short  0                   /* drvrMenu */
	.short  66f-60b             /* open */
	.short  62f-60b             /* prime */
	.short  63f-60b             /* control */
	.short  64f-60b             /* status */
	.short  65f-60b             /* close */
	.byte   61f-.-1             /* name length byte */
	.ascii  ".\myname"
61:
	.align 2
	.short 0x0100               /* driver v1.0 */

62:                             /* lots of illegal instructions to fill in later */
	.short  0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc
63:
	.short  0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc
64:
	.short  0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc
65:
	.short  0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc, 0x4afc

66:
	moveq   #0,%d4              /* no private heap (see below) */
	move.l  %a0,%a3             /* keep the PB and DCE for the driver */
	move.l  %a1,%a4

	/* Refuse to open without a device for this sResource: the Nth
	 * sResource of a type drives the Nth device of that type. PrimaryInit
	 * deletes the spare sResources, but on the Mac II family ROMs they
	 * stay (seen on the IIcx). The registers are at $Fs000000 in 32-bit mode
	 * and at $s00000 in 24-bit mode; the first device's magic number
	 * tells which. They are little-endian, hence the byte order here. */
	moveq   #0,%d0
	move.b  40(%a4),%d0         /* slot */
	move.l  %d0,%d1
	ror.l   #8,%d1
	add.l   #0xf0000000,%d1
	move.l  %d1,%a0
	cmp.l   #0x76697274,0x200(%a0) /* "virt" */
	beq.s   76f
	swap    %d0
	lsl.l   #4,%d0
	move.l  %d0,%a0             /* $s00000 */
76:
	lea     0x200(%a0),%a0      /* the first of 32 devices */
	moveq   #31,%d2
	moveq   #0,%d1              /* devices of our type */
77:
	cmp.l   #0x76697274,(%a0)   /* magic, version and ID, in that order */
	bne.s   78f
	cmp.l   #0x02000000,4(%a0)
	bne.s   78f
	cmp.l   #(\vtype)<<24,8(%a0)
	bne.s   78f
	addq.l  #1,%d1
78:
	lea     0x200(%a0),%a0
	dbf     %d2,77b
	moveq   #0,%d0
	move.b  41(%a4),%d0         /* sResource ID */
	sub.l   #\firstid,%d0
	cmp.l   %d1,%d0
	blt.s   79f
	moveq   #-23,%d0            /* openErr */
	rts
79:
	tst.b   0x0cb2              /* MMU32Bit */
	jbeq    72f

	moveq   #56/4-1,%d0         /* make an SpBlock */
67:
	clr.l   -(%sp)
	dbf     %d0,67b
	move.l  %sp,%a0
	move.b  40(%a4),49(%a0)     /* slot number into spBlock */
	move.b  41(%a4),50(%a0)     /* sResource number into spBlock */

	moveq   #11,%d0
	.short  0xa06e              /* call SGetSRsrc */
	move.l  4(%a0),%a2          /* this is the sResource pointer in ROM */
	lea     56(%sp),%sp
	jbra    70f

	/* In 24-bit mode the ROM, at the top of the slot, is out of reach:
	 * copy the driver into RAM and run it there. The copy is done in
	 * 32-bit mode with interrupts off. (SGetBlock would do it for us,
	 * but copies at most 64K.)
	 * During startup, while the system heap is still the only heap, the
	 * boot code keeps its stack just above that heap, so the heap must
	 * not grow by much: the copy, and a private heap that stands in for
	 * the system heap while the driver opens (it allocates ~300K), go
	 * below BufPtr at the top of RAM instead. */
72:
	move.l  0x02a6,%d1          /* SysZone */
	cmp.l   0x02aa,%d1          /* ApplZone */
	jbne    73f
	move.l  0x010c,%d1          /* BufPtr */
	sub.l   #\elfend-\elf,%d1
	and.w   #0xfff0,%d1
	move.l  %d1,%a2             /* the copy, at the top */
	sub.l   #StartupZoneSize,%d1
	move.l  %d1,0x010c          /* new BufPtr */
	move.l  %d1,%d4             /* the private heap, below the copy */
	move.l  0x0118,-(%sp)       /* TheZone, which InitZone changes */
	clr.l   -(%sp)              /* pGrowZone */
	move.w  #16,-(%sp)          /* cMoreMasters */
	move.l  %a2,-(%sp)          /* limitPtr */
	move.l  %d4,-(%sp)          /* startPtr */
	move.l  %sp,%a0
	.short  0xa019              /* InitZone */
	lea     14(%sp),%sp
	move.l  (%sp)+,0x0118
	jbra    74f
73:
	move.l  #\elfend-\elf,%d0
	.short  0xa51e              /* NewPtrSys */
	move.l  %a0,%d0
	jbeq    71f
	move.l  %a0,%a2             /* the copy */
74:
	clr.l   %d0
	move.b  40(%a4),%d0         /* slot */
	ror.l   #8,%d0
	add.l   #\elf-ROMEND+0xf1000000,%d0 /* the driver in ROM, 32-bit */
	move.l  %d0,%d2
	move.w  %sr,-(%sp)
	ori.w   #0x0700,%sr
	moveq   #1,%d0
	.short  0xa05d              /* SwapMMUMode to 32-bit, old mode in d0 */
	move.b  %d0,%d3
	move.l  %d2,%a0
	move.l  %a2,%a1
	move.l  #\elfend-\elf,%d0
	.short  0xa02e              /* BlockMove, which also keeps the caches right */
	move.b  %d3,%d0
	.short  0xa05d              /* SwapMMUMode back */
	move.w  (%sp)+,%sr

70:
	move.l  %a3,%a0
	move.l  %a4,%a1

68:
	cmp.w   #0x6004,(%a2)+      /* scan for magic number inside ELF code */
	bne.s   68b
	cmp.w   #0x6f70,(%a2)+
	bne.s   68b
	cmp.w   #0x656e,(%a2)+
	bne.s   68b
	move.l  0x02a6,-(%sp)       /* SysZone */
	tst.l   %d4
	beq.s   75f
	move.l  %d4,0x02a6          /* the private heap stands in for it */
75:
	jsr     (%a2)               /* the driver's Open */
	move.l  (%sp)+,0x02a6
	rts
71:
	moveq   #-108,%d0           /* memFullErr: fail the Open */
	rts
69:
	.endm


	.globl _start
	.section .text
_start:
ROMSTART:

/* The list of all the sResources on the ROM */
.set resNum, 128

sResourceDirectory:
	OSLstEntry 1, BoardResource

.rept 32 /* Maximum number of 9P devices, starting at sResource 128 */
	OSLstEntry resNum, Resource9P
.set resNum, resNum+1
.endr

.rept 32 /* Maximum number of block devices, starting at sResource 160 */
	OSLstEntry resNum, ResourceBlock
.set resNum, resNum+1
.endr

.rept 1 /* Maximum number of input devices */
OSLstEntry resNum, ResourceInput
.set resNum, resNum+1
.endr

.ifdef PDS_ETHERNET
	OSLstEntry 240, ResourceEthernet
.endif

	DatLstEntry endOfList, 0

/* Information applying to the "physical" board */
BoardResource:
	OSLstEntry sRsrcType, BoardTypeRec
	OSLstEntry sRsrcName, BoardName
	OSLstEntry sRsrcIcon, Icon
	OSLstEntry sRsrcCicn, Cicn
.ifdef PDS_ETHERNET
	DatLstEntry boardId, 93 /* Mac OS then uses its "Sonic 16" Ethernet driver */
.else
	DatLstEntry boardId, 0x9545
.endif
	OSLstEntry primaryInit, PrimaryInitRec
	OSLstEntry vendorInfo, VendorInfoRec
.ifdef PDS_ETHERNET
	DatLstEntry picOffset, 0x100 /* the SONIC's registers are at +0 */
.endif
	DatLstEntry endOfList, 0
BoardTypeRec:
	.short catBoard /* Category: Board! */
	.short typeBoard /* Type: Board! */
	.short 0 /* DrvrSw: 0, because it's a board */
	.short 0 /* DrvrHw: 0, because... it's a board */
BoardName:
.ifdef PDS_ETHERNET
	.asciz "Virtio bus and Ethernet"
.else
	.asciz "Virtio bus"
.endif
	.align 2
PrimaryInitRec:
	SlotExecROMStub PrimaryInitEnd
	SlotExecCShim
	.incbin "build/classic/standalone-primaryinit"
	.align 2
PrimaryInitEnd:

VendorInfoRec:
	OSLstEntry vendorId, VendorId
	OSLstEntry revLevel, RevLevel
	OSLstEntry partNum, PartNum
	DatLstEntry endOfList, 0
VendorId:
	.asciz "QEMU"
	.align 2
RevLevel:
	.asciz "1.0"
	.align 2
PartNum:
	.asciz "VirtioBus"
	.align 2

/* This is SolraBizna's icon, perhaps should change */
Icon:  .long 0x000FF000,0x007FFE00,0x01FFFF80,0x03E3FFC0,0x07C01FE0,0x0FC00FF0
	.long 0x1FC1CFF8,0x3F81C7FC,0x3F0001FC,0x7F03C07E,0x7F003F3E,0x7E0000BE
	.long 0xFE003FFF,0xFE007FFF,0xFE01FFFF,0xFE01FFFF,0xFF81FFFF,0xFFC0FFFF
	.long 0xFFF0FFFF,0xFFF87FFF,0x7FFC7FFE,0x7FFE7FFE,0x7FFE3FFE,0x3FFF3FFC
	.long 0x3FFF3FFC,0x1FFFBFF8,0x0FFFBFF0,0x07FFFFF8,0x03FFFFF8,0x01FFFFFC
	.long 0x007FFE7C,0x000FF03E
	/* Now in color! */
Cicn:  .long CicnEnd-Cicn
	.long 0x00000000,0x80100000,0x00000020,0x00200000,0x00000000,0x00000048
	.long 0x00000048,0x00000000,0x00040001,0x00040000,0x00000000,0x00000000
	.long 0x00000000,0x00000004,0x00000000,0x00200020,0x00000000,0x00040000
	.long 0x00000020,0x00200000,0x0000000F,0xF000007F,0xFE0001FF,0xFF8003FF
	.long 0xFFC007FF,0xFFE00FFF,0xFFF01FFF,0xFFF83FFF,0xFFFC3FFF,0xFFFC7FFF
	.long 0xFFFE7FFF,0xFFFE7FFF,0xFFFEFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFF7FFF,0xFFFE7FFF
	.long 0xFFFE7FFF,0xFFFE3FFF,0xFFFC3FFF,0xFFFC1FFF,0xFFF80FFF,0xFFF007FF
	.long 0xFFF803FF,0xFFF801FF,0xFFFC007F,0xFE7C000F,0xF03EFFFF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFE3,0xFFFFFFC0,0x1FFFFFC0,0x0FFFFFC1,0xCFFFFF81
	.long 0xC7FFFF00,0x01FFFF03,0xC07FFF00,0x3F3FFE00,0x00BFFE00,0x3FFFFE00
	.long 0x7FFFFE01,0xFFFFFE01,0xFFFFFF81,0xFFFFFFC0,0xFFFFFFF0,0xFFFFFFF8
	.long 0x7FFFFFFC,0x7FFFFFFE,0x7FFFFFFE,0x3FFFFFFF,0x3FFFFFFF,0x3FFFFFFF
	.long 0xBFFFFFFF,0xBFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xFFFF0000,0x00000000,0x000A0000,0xFFFF6666,0x33330001,0xCCCCCCCC
	.long 0xCCCC0002,0xBBBBBBBB,0xBBBB0003,0xAAAAAAAA,0xAAAA0004,0x88888888
	.long 0x88880005,0x77777777,0x77770006,0x55555555,0x55550007,0x44444444
	.long 0x44440008,0x22222222,0x22220009,0x11111111,0x1111000F,0x00000000
	.long 0x0000FFFF,0xFFFFFFFF,0x11111111,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFF12
	.long 0x12121212,0x12FFFFFF,0xFFFFFFFF,0xFFFF2222,0x22222222,0x2222FFFF
	.long 0xFFFFFFFF,0xFFF23230,0x00323232,0x32323FFF,0xFFFFFFFF,0xFF333300
	.long 0x00000003,0x333333FF,0xFFFFFFFF,0xFF434300,0x00000000,0x434343FF
	.long 0xFFFFFFFF,0xF4444400,0x000FFF00,0x4444444F,0xFFFFFFFF,0xF4545000
	.long 0x000FFF00,0x0454545F,0xFFFFFFFF,0xF5550000,0x00000000,0x0005555F
	.long 0xFFFFFFFF,0xF5650000,0x00FFFF00,0x0000056F,0xFFFFFFFF,0xFF660000
	.long 0x000000FF,0xFFFF00FF,0xFFFFFFFF,0xFF700000,0x00000000,0x0000F0FF
	.long 0xFFFFFFFF,0xFFF00000,0x00000077,0x77777FFF,0xFFFFFFFF,0xFFF00000
	.long 0x00000787,0x8787FFFF,0xFFFFFFFF,0xFFF00000,0x00088888,0x88FFFFFF
	.long 0xFFFFFFFF,0xFFF00000,0x00089898,0xFFFFFFFF,0xFFFFFFFF,0xFFFFF000
	.long 0x000FFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFF00,0x0000FFFF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF,0x0000FFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xF0000FFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFF000FFF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF,0xFFF00FFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xFFF000FF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFF00FF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF,0xFFFF00FF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xFFFFF0FF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFF0FF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	.long 0xFFFFFFFF,0xFFFFFFFF
	.byte 0xFF
	.align 2
CicnEnd:

Resource9P:
	OSLstEntry sRsrcType, 1$
	OSLstEntry sRsrcName, 2$
	OSLstEntry sRsrcDrvrDir, 3$
	OSLstEntry sRsrcBootRec, BootRec
	DatLstEntry sRsrcFlags, 2 /* open at start, use 32-bit addressing */
	DatLstEntry sRsrcHWDevId, 1
	DatLstEntry endOfList, 0
1$:
	.short catCPU
	.short typeDesk
	.short drSwMacCPU
	.short 0x5609
2$:
	.asciz "Virtio9P" /* without a leading dot */
	.align 2
3$:
	OSLstEntry sCPU_68020, 4$
	DatLstEntry endOfList, 0
4$:
	StubDriver Virtio9P, 5$, 6$, 128, 9
5$:
	.incbin "build/classic/drvr-9p.elf"
	.align 2
6$:

ResourceBlock:
	OSLstEntry sRsrcType, 1$
	OSLstEntry sRsrcName, 2$
	OSLstEntry sRsrcDrvrDir, 3$
	OSLstEntry sRsrcBootRec, BootRec
	DatLstEntry sRsrcFlags, 2 /* open at start, use 32-bit addressing */
	DatLstEntry sRsrcHWDevId, 1
	DatLstEntry endOfList, 0
1$:
	.short catCPU
	.short typeDesk
	.short drSwMacCPU
	.short 0x5602
2$:
	.asciz "VirtioBlock" /* without a leading dot */
	.align 2
3$:
	OSLstEntry sCPU_68020, 4$
	DatLstEntry endOfList, 0
4$:
	StubDriver VirtioBlock, 5$, 6$, 160, 2
5$:
	.incbin "build/classic/drvr-block.elf"
	.align 2
6$:

ResourceInput:
	OSLstEntry sRsrcType, 1$
	OSLstEntry sRsrcName, 2$
	OSLstEntry sRsrcDrvrDir, 3$
	DatLstEntry sRsrcFlags, 2 /* open at start, use 32-bit addressing */
	DatLstEntry sRsrcHWDevId, 1
	DatLstEntry endOfList, 0
1$:
	.short catCPU
	.short typeDesk
	.short drSwMacCPU
	.short 0x5612
2$:
	.asciz "VirtioInput" /* without a leading dot */
	.align 2
3$:
	OSLstEntry sCPU_68020, 4$
	DatLstEntry endOfList, 0
4$:
	StubDriver VirtioInput, 5$, 6$, 192, 18
5$:
	.incbin "build/classic/drvr-input.elf"
	.align 2
6$:

/* The LC processor-direct slot card with Ethernet: QEMU's
 * mac-pds-ethernet-virtio. Its SONIC and address PROM are where Apple's
 * "Sonic 16" driver expects them (registers at +0, PROM at +$40000); the
 * virtio devices stay at +$200, and the PIC moves to +$100. */
.ifdef PDS_ETHERNET
ResourceEthernet:
	OSLstEntry sRsrcType, 1$
	OSLstEntry sRsrcName, 2$
	DatLstEntry sRsrcHWDevId, 1
	DatLstEntry endOfList, 0
1$:
	.short catNetwork
	.short typeEthernet
	.short drSwApple
	.short 0x0119
2$:
	.asciz "Network_Ethernet_SONIC"
	.align 2
.endif

BootRec:
	SlotExecROMStub BootRecEnd
	SlotExecCShim
	.incbin "build/classic/standalone-boot"
	.align 2
BootRecEnd:

/* Pad so that, after the header, this ROM is a multiple of 4K */
/* Works around a suspected QEMU bug: */
/* Assertion failed: (!(iotlb & ~TARGET_PAGE_MASK)), function tlb_set_page_full */
	.org ((.+0xfff+0x14)&~0xfff)-0x14

/* Header, with magic number, must go at the end */
	.long (sResourceDirectory-.) & 0xffffff
	.long ROMEND-ROMSTART
	.long 0 /* Checksum goes here */
	.byte 1 /* ROM format revision */
	.byte 1 /* Apple format */
	.long 0x5A932BC7 /* Magic number */
	.byte 0 /* Must be zero */
	.byte 0x0F /* use all four byte lanes */
ROMEND:
	.end
