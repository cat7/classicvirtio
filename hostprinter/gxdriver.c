/* Licensed under the MIT license */

// 'pdvr' 0: the message overrides of the Host Printer GX driver
//
// QuickDraw GX's raster driver draws each page into 32-bit bands; they go
// to the host as rows of whole paper width. Nothing goes to a connection:
// the driver has none ('comm' type 'nops').

#include <Files.h>
#include <Folders.h>
#include <Memory.h>
#include <Resources.h>

#include "common.h"
#include "gx.h"

enum {
	kDPI = 150,
	kBestDPI = 300,             // as in the 'rdip'
	kBest = 1,                  // the first of the qualities
	kJobErr = -27,              // ioErr: GX reports the job as failed
};

struct Job {
	short ref;
	long dpi;
	Boolean failed;
	long paperW, paperH;        // pixels
	long offX, offY;            // where the page area is on the paper
	long mediaW, mediaH;        // 1/100 pt
};

static struct Job *job(void) {
	return GXGetInstanceContext();
}

// Fixed points, to pixels and to 1/100 pt (in 32 bits: 1/256 pt steps)
static long px(Fixed points, long dpi) {
	return ((points >> 8) * dpi / 72 + 128) >> 8;
}

static long hpt(Fixed points) {
	return ((points >> 8) * 100 + 128) >> 8;
}

// The resolution chosen with the Print dialog's Quality pop-up
static long jobDPI(void) {
	gxQualityInfo q;
	long size = sizeof q;

	memset(&q, 0, sizeof q);
	GetCollectionItem(GXGetJobCollection(GXGetJob()), gxQualityTag, gxPrintingTagID, &size, &q);
	return q.qualityCount == 2 && q.currentQuality == kBest ? kBestDPI : kDPI;
}

static void call(struct Job *j, struct PrintCall *c) {
	HostCall(j->ref, c);
	if (c->status != kPrintOK) j->failed = true;
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
	j->dpi = jobDPI();
	j->paperW = px(paper.right - paper.left, j->dpi);
	j->paperH = px(paper.bottom - paper.top, j->dpi);
	j->offX = px(page.left - paper.left, j->dpi);
	j->offY = px(page.top - paper.top, j->dpi);
	j->mediaW = hpt(paper.right - paper.left);
	j->mediaH = hpt(paper.bottom - paper.top);

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
	memset(&c, 0, sizeof c);
	c.op = kPrintEndPage;
	call(j, &c);
	return j->failed ? kJobErr : noErr;
}

// The band goes to the host as it is, in the background printer's memory,
// which has little to spare: the host puts it in place on the paper
OSErr HPRasterDataIn(gxOffscreenHdl offscreen, gxRectangle *band, gxRectangle *dirty) {
	struct Job *j = job();
	struct PrintCall c;
	if (!j || j->failed) return kJobErr;

	gxBitmap *bits = &(**offscreen).thePlanes[0].theBits;
	long top = (band->top + 0x8000) >> 16;
	long bottom = (band->bottom + 0x8000) >> 16;
	long n = bottom - top;
	if (n > bits->height) n = bits->height;
	long per = PRINT_MAX_DATA / bits->rowBytes;
	if (per < 1) return kJobErr;

	for (long r = 0; r < n && !j->failed; r += per) {
		long k = n - r < per ? n - r : per;
		memset(&c, 0, sizeof c);
		c.op = kPrintBand;
		c.a[0] = k;
		c.a[1] = bits->rowBytes;
		c.a[2] = j->offX;
		c.a[3] = bits->width;
		c.a[4] = j->offY + top + r + 1;
		c.data = bits->image + r * bits->rowBytes;
		c.datalen = k * bits->rowBytes;
		call(j, &c);
	}
	return j->failed ? kJobErr : noErr;
}

// 1/100 pt to Fixed points
static Fixed fx(long hpt) {
	return ((hpt / 100) << 16) + ((hpt % 100) << 16) / 100;
}

static void addQuality(gxJob job) {
	static const unsigned char names[] = "\x0e" "Best (300 dpi)" "\x10" "Normal (150 dpi)";
	Collection c = GXGetJobCollection(job);
	struct {
		gxQualityInfo q;
		char more[sizeof names];
	} item;
	long size = sizeof item;

	if (GetCollectionItem(c, gxQualityTag, gxPrintingTagID, &size, &item) == noErr &&
		item.q.qualityCount == 2) return;
	memset(&item, 0, sizeof item);
	item.q.defaultQuality = 2;
	item.q.currentQuality = 2;
	item.q.qualityCount = 2;
	memcpy(item.q.qualityNames, names, sizeof names - 1);
	AddCollectionItem(c, gxQualityTag, gxPrintingTagID,
		sizeof (gxQualityInfo) - 1 + sizeof names - 1, &item);
}

// GX's own papers (US Letter, US Legal, A4, B5, No. 10 Envelope): no need for files
static const long gxPapers[][2] = {
	{61200, 79200}, {61200, 100800}, {59528, 84189}, {49890, 70866}, {29700, 68400},
};

static Boolean gxHasPaper(long w, long h) {
	for (unsigned i = 0; i < sizeof gxPapers / sizeof *gxPapers; i++) {
		long dw = w - gxPapers[i][0], dh = h - gxPapers[i][1];
		if (dw > -150 && dw < 150 && dh > -150 && dh < 150) return true;
	}
	return false;
}

// A paper of the host as a 'ptyp' resource: its name, the page and the paper
static Handle paperType(StringPtr name, long w, long h, long l, long t, long r, long b) {
	long len = 1 + name[0];
	long nameLen = (len + 1) & ~1;
	Handle hd = NewHandleClear(nameLen + 32 + 4 + 4 + 4 + 4);
	if (!hd) return NULL;
	char *p = *hd;
	memcpy(p, name, len);
	Fixed *rect = (Fixed *)(p + nameLen);
	rect[0] = 0;                // the page: the paper less the margins
	rect[1] = 0;
	rect[2] = fx(w - l - r);
	rect[3] = fx(h - t - b);
	rect[4] = -fx(l);           // the paper, around the page
	rect[5] = -fx(t);
	rect[6] = fx(w - l);
	rect[7] = fx(h - t);
	p += nameLen + 32;
	memcpy(p + 4, "hstG", 4);   // base type 0: unknown; this driver's
	p[8] = 2;                   // in inches
	p[9] = 0xc0;                // old and new style
	return hd;                  // and no collection items
}

static OSErr deleteFile(short vref, long dir, StringPtr name) {
	HParamBlockRec pb;
	memset(&pb, 0, sizeof pb);
	pb.fileParam.ioNamePtr = name;
	pb.fileParam.ioVRefNum = vref;
	pb.fileParam.ioDirID = dir;
	return PBHDeleteSync(&pb);
}

static void setTypes(short vref, long dir, StringPtr name, OSType type, OSType creator) {
	HParamBlockRec pb;
	memset(&pb, 0, sizeof pb);
	pb.fileParam.ioNamePtr = name;
	pb.fileParam.ioVRefNum = vref;
	pb.fileParam.ioDirID = dir;
	if (PBHGetFInfoSync(&pb) != noErr) return;
	pb.fileParam.ioFlFndrInfo.fdType = type;
	pb.fileParam.ioFlFndrInfo.fdCreator = creator;
	pb.fileParam.ioDirID = dir;
	PBHSetFInfoSync(&pb);
}

// The files of the papers made last time, by their creator
static void removePaperFiles(short vref, long dir) {
	CInfoPBRec pb;
	Str63 name;

	for (short i = 1; ; ) {
		memset(&pb, 0, sizeof pb);
		pb.hFileInfo.ioNamePtr = name;
		pb.hFileInfo.ioVRefNum = vref;
		pb.hFileInfo.ioDirID = dir;
		pb.hFileInfo.ioFDirIndex = i;
		if (PBGetCatInfoSync(&pb) != noErr) break;
		if (!(pb.hFileInfo.ioFlAttrib & ioDirMask) &&
			pb.hFileInfo.ioFlFndrInfo.fdType == 'uspt' &&
			pb.hFileInfo.ioFlFndrInfo.fdCreator == 'hstG' &&
			deleteFile(vref, dir, name) == noErr) continue;   // the next one moved up
		i++;
	}
}

// QuickDraw GX takes the papers it offers from its own list and from paper
// type files in the Extensions folder, one paper to a file, named by the
// file: so the host printer's other papers become such files
static void writePaperFiles(void) {
	struct PrintCall c;
	short ref = HostOpen(), vref;
	long dir;

	memset(&c, 0, sizeof c);
	c.op = kPrintInfo;
	HostCall(ref, &c);
	if (c.status != kPrintOK) return;
	long n = c.b[1];
	if (FindFolder(kOnSystemDisk, kExtensionFolderType, kDontCreateFolder, &vref, &dir)) return;
	removePaperFiles(vref, dir);

	short saveRes = CurResFile();
	for (long k = 0; k < n; k++) {
		Str255 name;

		memset(&c, 0, sizeof c);
		c.op = kPrintMedia;
		c.a[0] = k;
		HostCall(ref, &c);
		if (c.status != kPrintOK) break;
		long w = c.b[0], h = c.b[1], l = c.b[2], t = c.b[3], r = c.b[4], b = c.b[5];
		if (w <= l + r || h <= t + b || gxHasPaper(w, h)) continue;
		PStrCopyC(name, c.payload, 31);
		for (int i = 1; i <= name[0]; i++) if (name[i] == ':') name[i] = '-';

		HCreateResFile(vref, dir, name);
		short file = HOpenResFile(vref, dir, name, fsRdWrPerm);
		if (file == -1) continue;
		Handle pt = paperType(name, w, h, l, t, r, b);
		if (pt) {
			AddResource(pt, 'ptyp', 9000, (StringPtr)"\p");
			WriteResource(pt);
		}
		CloseResFile(file);
		setTypes(vref, dir, name, 'uspt', 'hstG');
	}
	UseResFile(saveRes);
}

// A new desktop printer: the papers of the host printer
OSErr HPDefaultDesktopPrinter(StringPtr dtpName) {
	OSErr err = GXForward1(dtpName);
	if (!err) writePaperFiles();
	return err;
}

// The host takes any paper, but GX asks the user to feed a paper the tray
// does not have: "manual" feed by default, which needs no alert here
static void manualFeed(gxJob job) {
	struct { Boolean autoFeed; char pad; } feed = {false, 0};
	AddCollectionItem(GXGetJobCollection(job), gxPaperFeedTag, gxPrintingTagID,
		sizeof feed, &feed);
}

// A new job's printer: the qualities, and paper fed by "hand"
OSErr HPDefaultPrinter(gxPrinter printer) {
	OSErr err = GXForward1(printer);
	if (err) return err;
	gxJob job = GXGetPrinterJob(printer);
	if (GXGetJobPrinter(job) == GXGetJobOutputPrinter(job)) {
		addQuality(job);
		manualFeed(job);
	}
	return noErr;
}

// Normal quality images at 150 dpi, a quarter of the 'rdip' pixels
OSErr HPSetupImageData(gxRasterImageDataHdl data) {
	OSErr err = GXForward1(data);
	if (!err && jobDPI() != kBestDPI) {
		(**data).hImageRes = (Fixed)kDPI << 16;
		(**data).vImageRes = (Fixed)kDPI << 16;
	}
	return err;
}
