/* Licensed under the MIT license */

// Host printer: hands each request of the printer driver ("Host Printer"
// in the Chooser) to the device, which makes a PDF and prints it

#include <Devices.h>
#include <DriverServices.h>
#include <Errors.h>
#include <Memory.h>
#include <Types.h>

#include <string.h>

#include "allocator.h"
#include "cleanup.h"
#include "printcall.h"
#include "printf.h"
#include "transport.h"
#include "virtqueue.h"

#include "device.h"

struct wireReq {
	uint32_t op;
	uint32_t a[7];
} __attribute((scalar_storage_order("little-endian")));

struct wireResp {
	uint32_t status;
	uint32_t b[7];
	char payload[256];
} __attribute((scalar_storage_order("little-endian")));

enum {
	DATAPAGES = PRINT_MAX_DATA / 0x1000,
	RESPOFFSET = 0x800,
};

static char *pages;     // page 0: request and response; then the data
static uint32_t phys[1 + DATAPAGES];

DriverDescription TheDriverDescription = {
	kTheDescriptionSignature,
	kInitialDriverDescriptor,
	{"\x0cpci1af4,10c0", {0x00, 0x10, 0x80, 0x00}}, // v0.1
	{kDriverIsLoadedUponDiscovery |
		kDriverIsOpenedUponLoad,
		"\x0c.VirtioPrint"},
	{1, // nServices
	{{kServiceCategoryNdrvDriver, kNdrvTypeIsGeneric, {0x00, 0x10, 0x80, 0x00}}}} //v0.1
};

int DriverStop(void) {
	return noErr;
}

int DriverStart(short refNum) {
	InitLog();
	sprintf(LogPrefix, "Print(%d) ", refNum);

	if (!VInit(refNum)) {
		printf("Transport layer failure\n");
		goto openErr;
	}

	pages = AllocPages(1 + DATAPAGES, phys);
	if (pages == NULL) {
		printf("Memory allocation failure\n");
		goto openErr;
	}
	RegisterCleanupVoidPtr(FreePages, pages);

	if (!VFeaturesOK()) {
		printf("Feature negotiation failure\n");
		goto openErr;
	}

	VDriverOK();

	// a power of two, for the ring's index mask
	if (QInit(0, 32) < 2 + DATAPAGES) {
		printf("Virtqueue layer failure\n");
		goto openErr;
	}

	printf("Ready\n");
	return noErr;
openErr:
	VFail();
	return openErr;
}

static int transact(struct PrintCall *call) {
	struct wireReq *req = (void *)pages;
	struct wireResp *resp = (void *)(pages + RESPOFFSET);
	uint32_t addrs[2 + DATAPAGES], sizes[2 + DATAPAGES];
	uint32_t len = call->datalen;
	int n = 0;

	if (len > PRINT_MAX_DATA) return paramErr;

	req->op = call->op;
	for (int i=0; i<7; i++) req->a[i] = call->a[i];
	memset(resp, 0, sizeof *resp);
	if (len) BlockMoveData(call->data, pages + 0x1000, len);

	addrs[n] = phys[0];
	sizes[n++] = sizeof *req;
	for (int p=0; p*0x1000 < len; p++) {
		addrs[n] = phys[1 + p];
		sizes[n++] = (len - p*0x1000 < 0x1000) ? len - p*0x1000 : 0x1000;
	}
	addrs[n] = phys[0] + RESPOFFSET;
	sizes[n++] = sizeof *resp;

	QSend(0, n-1, 1, addrs, sizes, NULL, true);

	call->status = resp->status;
	for (int i=0; i<7; i++) call->b[i] = resp->b[i];
	memcpy(call->payload, resp->payload, sizeof call->payload);
	return noErr;
}

int DriverRead(IOParam *pb) {
	return readErr;
}

int DriverWrite(IOParam *pb) {
	return writErr;
}

int DriverCtl(CntrlParam *pb) {
	if (pb->csCode == PRINT_CSCODE) {
		struct PrintCall *call;
		memcpy(&call, pb->csParam, sizeof call);
		return transact(call);
	}
	return controlErr;
}

int DriverStatus(CntrlParam *pb) {
	return statusErr;
}

void DNotified(uint16_t q, volatile uint32_t *retlen) {
}

void DConfigChange(void) {
}
