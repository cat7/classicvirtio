/* Licensed under the MIT license */

/*
The card's registers in the current addressing mode. In 32-bit mode the
slot is at $Fs000000. In 24-bit mode only the first megabyte of the slot
is reachable, at $s00000; $Fs000000 is then not the slot at all (on the
68030 and 68040 Macs it is RAM, mirrored every 16 MB). All the registers
are in that first megabyte.

The mode is found by looking for the first device's magic number at
$Fs000200, so it does not depend on when MMU32Bit gets set. Checked at
every access, because code that runs at interrupt time can find the Mac
in either mode.
*/

#pragma once

// Always inlined: standalone-primaryinit.c must start with its entry point
static inline __attribute__((always_inline)) unsigned long SlotBase(int slot) {
	unsigned long base32 = 0xf0000000UL + ((unsigned long)slot << 24);
	if (*(volatile unsigned long *)(base32 + 0x200) == 0x76697274) // "virt", little-endian register read raw
		return base32;
	return (unsigned long)slot << 20;
}
