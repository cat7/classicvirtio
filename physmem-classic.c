/* Licensed under the MIT license */

#include <MacErrors.h>
#include <Memory.h>
#include <OSUtils.h>
#include <Patches.h>

#include <stdbool.h>

// The real calls, before physmem.h renames them
static OSErr realLock(void *a, unsigned long n) { return LockMemory(a, n); }
static OSErr realUnlock(void *a, unsigned long n) { return UnlockMemory(a, n); }
static OSErr realGetPhysical(LogicalToPhysicalTable *t, unsigned long *n) { return GetPhysical(t, n); }

#include "physmem.h"

#define _MemoryDispatch 0xA05C
#define _Unimplemented 0xA89F

static bool haveDispatch(void) {
	static int known = -1;
	if (known < 0) {
		known = NGetTrapAddress(_MemoryDispatch, OSTrap) !=
			NGetTrapAddress(_Unimplemented, ToolTrap);
	}
	return known;
}

OSErr SafeLockMemory(void *address, unsigned long count) {
	return haveDispatch() ? realLock(address, count) : noErr;
}

OSErr SafeUnlockMemory(void *address, unsigned long count) {
	return haveDispatch() ? realUnlock(address, count) : noErr;
}

// Same contract as GetPhysical: entry 0 is the logical range, which is
// advanced past what was translated; entries 1.. receive physical extents.
OSErr SafeGetPhysical(LogicalToPhysicalTable *table, unsigned long *count) {
	if (haveDispatch()) return realGetPhysical(table, count);

	MemoryBlock *t = (MemoryBlock *)table;
	if (*count == 0) return paramErr;
	t[1].address = StripAddress(t[0].address);
	t[1].count = t[0].count;
	t[0].address = (char *)t[0].address + t[0].count;
	t[0].count = 0;
	*count = 1;
	return noErr;
}
