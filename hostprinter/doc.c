/* Licensed under the MIT license */

// PDEF 0 (and 1): PrOpenDoc, PrCloseDoc, PrOpenPage, PrClosePage
//
// Each page is recorded as a picture, then drawn into 32-bit offscreen
// bands at the chosen resolution and sent to the host band by band.

#include <Devices.h>
#include <Events.h>
#include <Gestalt.h>
#include <LowMem.h>
#include <Memory.h>
#include <QDOffscreen.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#include <Windows.h>

#include "common.h"

struct Doc {
	short ref;
	short page;             // the number of the open page, from 1
	short first, last;      // the range to print
	short dpi, res;
	long mediaW, mediaH;    // 1/100 pt, as printed (turned if landscape)
	Rect rPage, rPaper;
	PicHandle pic;
	Rect frame;
	THPrint hPrint;
	PrIdleUPP idle;
	Boolean color;          // a color port
	Boolean skip;           // this page is outside the range
	Boolean failed, aborted, started, sentPage;
	Str255 message;
};

enum {
	kAlertID = -8192,
	kBandBytes = 0x200000,  // the most one band takes
};

static struct Doc *docOf(TPPrPort port) {
	return (struct Doc *)port->lGParam1;
}

static void setErr(short err) {
	if (PrintErrGlobal == noErr) PrintErrGlobal = err;
}

static void call(struct Doc *d, struct PrintCall *c) {
	HostCall(d->ref, c);
}

static Boolean userCancelled(void) {
	EventRecord ev;
	if (EventAvail(keyDownMask | autoKeyMask, &ev) &&
		(ev.modifiers & cmdKey) && (ev.message & charCodeMask) == '.') {
		GetNextEvent(keyDownMask | autoKeyMask, &ev);
		return true;
	}
	return false;
}

static void fail(struct Doc *d, const char *why) {
	if (!d->failed && why && !d->message[0]) PStrCopyC(d->message, why, 255);
	d->failed = true;
	setErr(iIOAbort);
}

static void startJob(struct Doc *d) {
	struct PrintCall c;
	Str255 title;
	char ctitle[256];
	WindowPtr w = FrontWindow();

	title[0] = 0;
	if (w) GetWTitle(w, title);
	if (!title[0]) {
		StringPtr app = LMGetCurApName();
		memcpy(title, app, app[0] + 1);
	}
	memcpy(ctitle, title + 1, title[0]);
	ctitle[title[0]] = 0;

	memset(&c, 0, sizeof c);
	c.op = kPrintBeginJob;
	c.a[0] = (**d->hPrint).prJob.iCopies;
	c.data = ctitle;
	c.datalen = title[0] + 1;
	call(d, &c);
	if (c.status == kPrintNoDevice) {
		fail(d, "The Host Printer is not connected.");
	} else if (c.status != kPrintOK) {
		fail(d, "The host cannot start the job.");
	} else {
		d->started = true;
	}
}

// Pascal: FUNCTION PrOpenDoc(hPrint: THPrint; pPrPort: TPPrPort; pIOBuf: Ptr): TPPrPort;
struct OpenDocArgs {Ptr pIOBuf; TPPrPort port; THPrint hPrint;};
TPPrPort PrOpenDocC(struct OpenDocArgs *a) {
	THPrint h = a->hPrint;
	TPPrPort port = a->port;
	struct Doc *d;
	long qd = 0;

	PrintErrGlobal = noErr;
	d = (struct Doc *)NewPtrClear(sizeof (struct Doc));
	if (!d) {
		setErr(iMemFullErr);
		return NULL;
	}
	if (!port) {
		port = (TPPrPort)NewPtrClear(sizeof (TPrPort));
		if (!port) {
			DisposePtr((Ptr)d);
			setErr(iMemFullErr);
			return NULL;
		}
		port->fOurPtr = true;
	} else {
		port->fOurPtr = false;
	}
	port->fOurBits = false;
	port->lGParam1 = (long)d;

	struct Settings *s = Sets(h);
	if (s->magic != kMagic) DefaultRecord(h);
	Geometry(h);
	s = Sets(h);

	d->hPrint = h;
	d->ref = HostOpen();
	d->dpi = s->dpi ? s->dpi : kDefaultDPI;
	d->res = s->res ? s->res : 72;
	d->mediaW = s->orient ? s->height : s->width;
	d->mediaH = s->orient ? s->width : s->height;
	d->rPage = (**h).prInfo.rPage;
	d->rPaper = (**h).rPaper;
	d->first = (**h).prJob.iFstPage;
	d->last = (**h).prJob.iLstPage;
	if (d->first < 1) d->first = 1;
	if (d->last < d->first) d->last = d->first;
	d->idle = (**h).prJob.pIdleProc;

	Gestalt(gestaltQuickdrawVersion, &qd);
	d->color = qd >= gestalt8BitQD;
	if (d->color) {
		OpenCPort((CGrafPtr)&port->gPort);
	} else {
		OpenPort(&port->gPort);
	}
	port->gPort.portRect = d->rPage;
	RectRgn(port->gPort.visRgn, &d->rPaper);
	SetPort(&port->gPort);

	startJob(d);
	return port;
}

// Pascal: PROCEDURE PrOpenPage(pPrPort: TPPrPort; pPageFrame: TPRect);
struct OpenPageArgs {TPRect frame; TPPrPort port;};
void PrOpenPageC(struct OpenPageArgs *a) {
	struct Doc *d = docOf(a->port);

	SetPort(&a->port->gPort);
	d->page++;
	d->skip = d->page < d->first || d->page > d->last || d->failed || d->aborted;
	d->frame = a->frame ? *a->frame : d->rPage;
	ClipRect(&d->rPaper);
	d->pic = OpenPicture(&d->frame);
}

static void sendPage(struct Doc *d) {
	struct PrintCall c;
	long w = (d->mediaW * d->dpi + 3600) / 7200;
	long h = (d->mediaH * d->dpi + 3600) / 7200;
	long rows = kBandBytes / (w * 4);
	GWorldPtr gw = NULL;
	CGrafPtr savePort;
	GDHandle saveDev;
	Rect band, dest;
	OSErr err = -1;

	if (rows > h) rows = h;
	while (rows >= 8) {
		SetRect(&band, 0, 0, w, rows);
		err = NewGWorld(&gw, 32, &band, NULL, NULL, 0);
		if (err) err = NewGWorld(&gw, 32, &band, NULL, NULL, useTempMem);
		if (!err) break;
		rows /= 2;
	}
	if (err) {
		fail(d, "There is not enough memory to print this page.");
		return;
	}

	// The page frame goes where rPage is on the paper
	dest.left = (long)(d->rPage.left - d->rPaper.left) * d->dpi / d->res;
	dest.top = (long)(d->rPage.top - d->rPaper.top) * d->dpi / d->res;
	dest.right = (long)(d->rPage.right - d->rPaper.left) * d->dpi / d->res;
	dest.bottom = (long)(d->rPage.bottom - d->rPaper.top) * d->dpi / d->res;

	memset(&c, 0, sizeof c);
	c.op = kPrintBeginPage;
	c.a[0] = w;
	c.a[1] = h;
	c.a[2] = d->mediaW;
	c.a[3] = d->mediaH;
	call(d, &c);
	if (c.status != kPrintOK) {
		DisposeGWorld(gw);
		fail(d, "The host cannot take the page.");
		return;
	}

	GetGWorld(&savePort, &saveDev);
	PixMapHandle pm = GetGWorldPixMap(gw);
	LockPixels(pm);
	long rb = (**pm).rowBytes & 0x3fff;
	long chunk = PRINT_MAX_DATA / rb;

	for (long y0 = 0; y0 < h && !d->failed; y0 += rows) {
		long n = h - y0 < rows ? h - y0 : rows;

		SetGWorld(gw, NULL);
		SetOrigin(0, y0);
		ClipRect(&gw->portRect);
		ForeColor(blackColor);
		BackColor(whiteColor);
		EraseRect(&gw->portRect);
		DrawPicture(d->pic, &dest);
		SetOrigin(0, 0);
		SetGWorld(savePort, saveDev);

		char *base = GetPixBaseAddr(pm);
		for (long r = 0; r < n; r += chunk) {
			long k = n - r < chunk ? n - r : chunk;
			memset(&c, 0, sizeof c);
			c.op = kPrintBand;
			c.a[0] = k;
			c.a[1] = rb;
			c.data = base + r * rb;
			c.datalen = k * rb;
			call(d, &c);
			if (c.status != kPrintOK) {
				fail(d, "The host lost the page.");
				break;
			}
		}

		if (d->idle) CallPrIdleProc(d->idle);
		if (userCancelled()) {
			d->aborted = true;
			setErr(iPrAbort);
			break;
		}
	}
	UnlockPixels(pm);
	DisposeGWorld(gw);

	if (d->failed || d->aborted) return;
	memset(&c, 0, sizeof c);
	c.op = kPrintEndPage;
	call(d, &c);
	if (c.status != kPrintOK) fail(d, "The host could not finish the page.");
	else d->sentPage = true;
}

// Pascal: PROCEDURE PrClosePage(pPrPort: TPPrPort);
struct ClosePageArgs {TPPrPort port;};
void PrClosePageC(struct ClosePageArgs *a) {
	struct Doc *d = docOf(a->port);

	SetPort(&a->port->gPort);
	ClosePicture();
	if (!d->pic) return;
	if (!d->skip && d->started && !d->failed && !d->aborted) {
		if (HandleBytes((Handle)d->pic) <= sizeof (Picture)) {
			fail(d, "There is not enough memory to record this page.");
		} else {
			HLock((Handle)d->pic);
			sendPage(d);
			HUnlock((Handle)d->pic);
		}
	}
	KillPicture(d->pic);
	d->pic = NULL;
}

// Pascal: PROCEDURE PrCloseDoc(pPrPort: TPPrPort);
struct CloseDocArgs {TPPrPort port;};
void PrCloseDocC(struct CloseDocArgs *a) {
	TPPrPort port = a->port;
	struct Doc *d = docOf(port);
	struct PrintCall c;

	if (d->started) {
		memset(&c, 0, sizeof c);
		if (d->failed || d->aborted || !d->sentPage) {
			c.op = kPrintAbort;
			call(d, &c);
		} else {
			c.op = kPrintEndJob;
			call(d, &c);
			if (c.status != kPrintOK) {
				d->failed = true;
				PStrCopyC(d->message, c.payload, 127);
				setErr(iIOAbort);
			}
		}
	}

	if (d->color) {
		CloseCPort((CGrafPtr)&port->gPort);
	} else {
		ClosePort(&port->gPort);
	}

	if (d->failed) {
		Str255 empty = {0};
		if (!d->message[0]) PStrCopyC(d->message, "The page could not be printed.", 255);
		ParamText(d->message, empty, empty, empty);
		StopAlert(kAlertID, NULL);
	}

	DisposePtr((Ptr)d);
	if (port->fOurPtr) DisposePtr((Ptr)port);
}
