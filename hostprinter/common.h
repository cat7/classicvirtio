/* Licensed under the MIT license */

// Shared by the code resources of the Host Printer driver

#pragma once

#include <Dialogs.h>
#include <Printing.h>
#include <Quickdraw.h>
#include <Types.h>

#include <stdint.h>

#include "../printcall.h"

// Kept in the print record's spare words (printX, 38 bytes)
struct Settings {
	short magic;            // kMagic
	short orient;           // 0 portrait, 1 landscape
	long width, height;     // the paper, 1/100 pt, portrait
	long margin[4];         // left, top, right, bottom, 1/100 pt, portrait
	short res;              // the application's resolution (dpi)
	short dpi;              // rendering resolution
	short media;            // index in the host's list
	short pad;
};

enum {
	kMagic = 0x4850,        // 'HP'
	kDevID = 0x7700,        // prStl.wDev: no printer the applications know
	kDefaultDPI = 150,
};

// Low memory
#define PrintErrGlobal (*(volatile short *)0x0944)

struct Settings *Sets(THPrint h);

// _GetHandleSize, without the library's glue
static inline long HandleBytes(Handle h) {
	long r;
	__asm__ volatile("move.l %1,%%a0\n\t.short 0xa025\n\tmove.l %%d0,%0"
		: "=d"(r) : "g"(h) : "d0", "d1", "d2", "a0", "a1", "cc", "memory");
	return r;
}

// The .VirtioPrint driver, 0 if there is none
short HostOpen(void);
// One request; the answer's status is kPrintNoDevice without the driver
void HostCall(short ref, struct PrintCall *c);
// INFO then MEDIA(default): fills the paper in s; returns false without a host
Boolean HostDefaults(struct Settings *s);

// The page geometry of a print record from its settings
void Geometry(THPrint h);
// Defaults, then Geometry
void DefaultRecord(THPrint h);

// Pascal strings
void PStrCopyC(Str255 dst, const char *src, int max);
void PStrAppendC(Str255 dst, const char *src);
void PStrAppendNum(Str255 dst, long n);

void *memcpy(void *dst, const void *src, unsigned long n);
void *memset(void *dst, int c, unsigned long n);
