/* Licensed under the MIT license */

/*
LockMemory, UnlockMemory and GetPhysical all go through the
_MemoryDispatch trap. System 7 installs it when the machine has
virtual memory, and on some Macs without it (the Mac II family) the
trap is missing: calling it then stops the Mac with "unimplemented
trap". Without virtual memory, logical and physical addresses are the
same, so these stand-ins do without the trap there.

PowerPC drivers always have the trap and use the real calls.
*/

#pragma once

#include <Memory.h>

#if defined(__m68k__)

OSErr SafeLockMemory(void *address, unsigned long count);
OSErr SafeUnlockMemory(void *address, unsigned long count);
OSErr SafeGetPhysical(LogicalToPhysicalTable *table, unsigned long *count);

#define LockMemory SafeLockMemory
#define UnlockMemory SafeUnlockMemory
#define GetPhysical SafeGetPhysical

#endif
