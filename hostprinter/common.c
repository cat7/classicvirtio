/* Licensed under the MIT license */

#include <Devices.h>
#include <Files.h>
#include <Memory.h>

#include "common.h"

void *memcpy(void *dst, const void *src, unsigned long n) {
	char *d = dst;
	const char *s = src;
	while (n--) *d++ = *s++;
	return dst;
}

void *memset(void *dst, int c, unsigned long n) {
	char *d = dst;
	while (n--) *d++ = c;
	return dst;
}

struct Settings *Sets(THPrint h) {
	return (struct Settings *)(**h).printX;
}

short HostOpen(void) {
	ParamBlockRec pb;
	memset(&pb, 0, sizeof pb);
	pb.ioParam.ioNamePtr = (StringPtr)PRINT_DRVR_NAME;
	pb.ioParam.ioPermssn = fsCurPerm;
	if (PBOpenSync(&pb) != noErr) return 0;
	return pb.ioParam.ioRefNum;
}

void HostCall(short ref, struct PrintCall *c) {
	CntrlParam pb;

	c->status = kPrintNoDevice;
	if (ref == 0) return;
	memset(&pb, 0, sizeof pb);
	pb.ioCRefNum = ref;
	pb.csCode = PRINT_CSCODE;
	memcpy(pb.csParam, &c, sizeof c);
	if (PBControlSync((ParmBlkPtr)&pb) != noErr) c->status = kPrintNoDevice;
}

static void letter(struct Settings *s) {
	s->width = 61200;
	s->height = 79200;
	for (int i=0; i<4; i++) s->margin[i] = 1800;
	s->media = 0;
}

Boolean HostDefaults(struct Settings *s) {
	struct PrintCall c;
	short ref = HostOpen();

	letter(s);
	memset(&c, 0, sizeof c);
	c.op = kPrintInfo;
	HostCall(ref, &c);
	if (c.status != kPrintOK) return false;

	short def = c.b[2];
	memset(&c, 0, sizeof c);
	c.op = kPrintMedia;
	c.a[0] = def;
	HostCall(ref, &c);
	if (c.status == kPrintOK && c.b[0] && c.b[1]) {
		s->width = c.b[0];
		s->height = c.b[1];
		for (int i=0; i<4; i++) s->margin[i] = c.b[2+i];
		s->media = def;
	}
	return true;
}

// 1/100 pt to the units of res dpi
static short toRes(long hpt, short res) {
	return (hpt * res + 3600) / 7200;
}

void Geometry(THPrint h) {
	struct Settings *s = Sets(h);
	long w = s->width, ht = s->height;
	long l = s->margin[0], t = s->margin[1], r = s->margin[2], b = s->margin[3];
	short res = s->res ? s->res : 72;

	if (s->orient) { // the paper turned a quarter clockwise
		long tmp = w; w = ht; ht = tmp;
		long ol = l, ot = t, or = r, ob = b;
		l = ob; t = ol; r = ot; b = or;
	}

	TPPrint p = *h;
	p->iPrVersion = iPrRelease;
	p->prInfo.iDev = 0; // the screen's fonts
	p->prInfo.iVRes = res;
	p->prInfo.iHRes = res;
	SetRect(&p->prInfo.rPage, 0, 0, toRes(w - l - r, res), toRes(ht - t - b, res));
	SetRect(&p->rPaper, -toRes(l, res), -toRes(t, res),
		toRes(w - l, res), toRes(ht - t, res));
	p->prStl.wDev = kDevID;
	p->prStl.iPageV = (ht * iPrPgFract + 3600) / 7200;
	p->prStl.iPageH = (w * iPrPgFract + 3600) / 7200;
	p->prStl.bPort = 0;
	p->prStl.feed = feedCut;
	p->prInfoPT = p->prInfo;
	memset(&p->prXInfo, 0, sizeof p->prXInfo);
	p->prXInfo.iBandV = p->prInfo.rPage.bottom;
	p->prXInfo.iBandH = p->prInfo.rPage.right;
	p->prXInfo.iBands = 1;
	p->prJob.bJDocLoop = bDraftLoop;
}

void DefaultRecord(THPrint h) {
	struct Settings *s;
	char state = HGetState((Handle)h);

	HLock((Handle)h);
	memset(*h, 0, sizeof (TPrint));
	s = Sets(h);
	HostDefaults(s);
	s->magic = kMagic;
	s->res = 72;
	s->dpi = kDefaultDPI;
	TPPrint p = *h;
	p->prJob.iFstPage = 1;
	p->prJob.iLstPage = iPrPgMax;
	p->prJob.iCopies = 1;
	p->prJob.fFromUsr = true;
	Geometry(h);
	HSetState((Handle)h, state);
}

void PStrCopyC(Str255 dst, const char *src, int max) {
	int n = 0;
	while (src[n] && n < max && n < 255) {
		dst[n+1] = src[n];
		n++;
	}
	dst[0] = n;
}

void PStrAppendC(Str255 dst, const char *src) {
	while (*src && dst[0] < 255) dst[++dst[0]] = *src++;
}

void PStrAppendNum(Str255 dst, long n) {
	char buf[12];
	int i = 0;
	if (n < 0) {
		PStrAppendC(dst, "-");
		n = -n;
	}
	do {
		buf[i++] = '0' + n % 10;
		n /= 10;
	} while (n);
	while (i--) {
		char c[2] = {buf[i], 0};
		PStrAppendC(dst, c);
	}
}
