/* Licensed under the MIT license */

// 'pdvr' 0: the message overrides of the Host Printer GX driver
//
// QuickDraw GX's raster driver draws each page into 32-bit bands; they go
// to the host as rows of whole paper width. Nothing goes to a connection:
// the driver has none ('comm' type 'nops').

#include <Memory.h>

#include "common.h"
#include "gx.h"

enum {
	kDPI = 150,                 // as in the 'rdip'
	kJobErr = -27,              // ioErr: GX reports the job as failed
};

struct Job {
	short ref;
	Boolean failed;
	long paperW, paperH;        // pixels
	long offX, offY;            // where the page area is on the paper
	long mediaW, mediaH;        // 1/100 pt
	long rows;                  // rows sent of this page
	long stride, chunkMax, chunkRows;
	char *chunk;
};

static struct Job *job(void) {
	return GXGetInstanceContext();
}

// Fixed points, to pixels and to 1/100 pt (in 32 bits: 1/256 pt steps)
static long px(Fixed points) {
	return ((points >> 8) * kDPI / 72 + 128) >> 8;
}

static long hpt(Fixed points) {
	return ((points >> 8) * 100 + 128) >> 8;
}

static void call(struct Job *j, struct PrintCall *c) {
	HostCall(j->ref, c);
	if (c->status != kPrintOK) j->failed = true;
}

static void flush(struct Job *j) {
	struct PrintCall c;
	if (!j->chunkRows || j->failed) return;
	memset(&c, 0, sizeof c);
	c.op = kPrintBand;
	c.a[0] = j->chunkRows;
	c.a[1] = j->stride;
	c.data = j->chunk;
	c.datalen = j->chunkRows * j->stride;
	call(j, &c);
	j->chunkRows = 0;
}

// One row of the paper: pixels at the page area's left, or white
static void addRow(struct Job *j, const char *pixels, long width) {
	char *row;
	if (j->rows >= j->paperH || j->failed) return;
	row = j->chunk + j->chunkRows * j->stride;
	memset(row, 0xff, j->stride);
	if (pixels) {
		if (width > j->paperW - j->offX) width = j->paperW - j->offX;
		if (width > 0) memcpy(row + j->offX * 4, pixels, width * 4);
	}
	j->rows++;
	if (++j->chunkRows == j->chunkMax) flush(j);
}

// The job starts with the connection: StartJob and FinishJob belong to the
// spooling, in the application, and the imaging happens later elsewhere
OSErr HPOpenConnection(void) {
	struct PrintCall c;
	char title[64];
	gxJobInfo info;
	long size = sizeof info;

	struct Job *j = (struct Job *)NewPtrClear(sizeof (struct Job));
	if (!j) return memFullErr;
	GXSetInstanceContext(j);
	j->ref = HostOpen();

	memset(&info, 0, sizeof info);
	GetCollectionItem(GXGetJobCollection(GXGetJob()), 'job ', gxPrintingTagID, &size, &info);
	memcpy(title, info.documentName + 1, info.documentName[0]);
	title[info.documentName[0]] = 0;

	memset(&c, 0, sizeof c);
	c.op = kPrintBeginJob;
	c.a[0] = 1;                 // GX images each copy
	c.data = title;
	c.datalen = info.documentName[0] + 1;
	call(j, &c);
	return j->failed ? kJobErr : noErr;
}

OSErr HPCloseConnection(void) {
	struct Job *j = job();
	struct PrintCall c;
	OSErr err = noErr;

	if (j) {
		memset(&c, 0, sizeof c);
		c.op = j->failed ? kPrintAbort : kPrintEndJob;
		HostCall(j->ref, &c);
		if (c.status != kPrintOK && !j->failed) err = kJobErr;
		if (j->chunk) DisposePtr(j->chunk);
		DisposePtr((Ptr)j);
		GXSetInstanceContext(NULL);
	}
	return err;
}

OSErr HPWriteData(Ptr data, long length) {
	return noErr;
}

OSErr HPStartSendPage(gxFormat format) {
	struct Job *j = job();
	struct PrintCall c;
	gxRectangle page, paper;

	if (!j || j->failed) return kJobErr;
	GXGetFormatDimensions(format, &page, &paper);
	j->paperW = px(paper.right - paper.left);
	j->paperH = px(paper.bottom - paper.top);
	j->offX = px(page.left - paper.left);
	j->offY = px(page.top - paper.top);
	j->mediaW = hpt(paper.right - paper.left);
	j->mediaH = hpt(paper.bottom - paper.top);
	j->rows = 0;
	j->chunkRows = 0;

	if (j->stride != j->paperW * 4) {
		if (j->chunk) DisposePtr(j->chunk);
		j->stride = j->paperW * 4;
		j->chunkMax = PRINT_MAX_DATA / j->stride;
		j->chunk = NewPtr(j->chunkMax * j->stride);
		if (!j->chunk) {
			j->failed = true;
			return memFullErr;
		}
	}

	memset(&c, 0, sizeof c);
	c.op = kPrintBeginPage;
	c.a[0] = j->paperW;
	c.a[1] = j->paperH;
	c.a[2] = j->mediaW;
	c.a[3] = j->mediaH;
	call(j, &c);
	return j->failed ? kJobErr : noErr;
}

OSErr HPFinishSendPage(void) {
	struct Job *j = job();
	struct PrintCall c;

	if (!j || j->failed) return kJobErr;
	flush(j);
	memset(&c, 0, sizeof c);
	c.op = kPrintEndPage;
	call(j, &c);
	return j->failed ? kJobErr : noErr;
}

OSErr HPRasterDataIn(gxOffscreenHdl offscreen, gxRectangle *band, gxRectangle *dirty) {
	struct Job *j = job();
	if (!j || j->failed) return kJobErr;

	gxBitmap *bits = &(**offscreen).thePlanes[0].theBits;
	long top = (band->top + 0x8000) >> 16;
	long bottom = (band->bottom + 0x8000) >> 16;
	long n = bottom - top;
	if (n > bits->height) n = bits->height;

	while (j->rows < j->offY + top && !j->failed) addRow(j, NULL, 0);
	for (long r = 0; r < n && !j->failed; r++) {
		addRow(j, bits->image + r * bits->rowBytes, bits->width);
	}
	return j->failed ? kJobErr : noErr;
}
