/* Licensed under the MIT license */

// The .VirtioPrint driver's one Control call, shared with the printer driver

#pragma once

#include <stdint.h>

#define PRINT_DRVR_NAME "\p.VirtioPrint"
#define PRINT_CSCODE 128       // csParam holds a struct PrintCall *
#define PRINT_MAX_DATA 0x10000 // bytes of data per call

enum {
	kPrintInfo = 0,       // -> payload: name[64], message[128]
	kPrintMedia = 1,      // a0 index -> payload: name[64]
	kPrintBeginJob = 2,   // a0 copies, data: title (C string)
	kPrintBeginPage = 3,  // a0 width px, a1 height px, a2 w, a3 h (1/100 pt)
	kPrintBand = 4,       // a0 rows, a1 bytes per row, data: the rows
	kPrintEndPage = 5,
	kPrintEndJob = 6,     // -> payload: message[128]
	kPrintAbort = 7,
};

enum {
	kPrintOK = 0,
	kPrintErr = 1,
	kPrintNoMedia = 2,
	kPrintBadState = 3,
	kPrintNoDevice = 99,  // set by the caller: no .VirtioPrint
};

// printer states, as IPP
enum {
	kPrinterIdle = 3,
	kPrinterBusy = 4,
	kPrinterStopped = 5,
};

// INFO:    b0 state, b1 media count, b2 default media, b3 color
// MEDIA:   b0 width, b1 height, b2 left, b3 top, b4 right, b5 bottom
//          (1/100 pt; the margins are what the printer cannot reach)
// END_JOB: b0 host job id
struct PrintCall {
	uint32_t op;
	uint32_t a[7];
	const void *data;
	uint32_t datalen;
	uint32_t status;      // out
	uint32_t b[7];        // out
	char payload[256];    // out
};
