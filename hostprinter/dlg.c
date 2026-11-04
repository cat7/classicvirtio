/* Licensed under the MIT license */

// PDEF 4: PrintDefault, PrStlDialog, PrJobDialog, PrStlInit, PrJobInit,
// PrDlgMain, PrValidate, PrJobMerge

#include <ControlDefinitions.h>
#include <Controls.h>
#include <Dialogs.h>
#include <Errors.h>
#include <Memory.h>
#include <Menus.h>
#include <TextUtils.h>
#include <ToolUtils.h>

#include "common.h"

enum {
	kStlDialog = -8192,
	kJobDialog = -8191,
	kMaxMedia = 32,
	kDlgMagic = 0x48504467,
};

// Page Setup items
enum {
	sOK = 1, sCancel, sPaper, sPortrait, sLandscape, sOrientLabel, sPrinter,
};

// Print items
enum {
	jOK = 1, jCancel, jCopiesLabel, jCopies, jPagesLabel, jAll, jFrom,
	jFirst, jToLabel, jLast, jQuality, jPrinter, jStatus,
};

struct MyDlg {
	TPrDlg pd;
	long magic;
	struct Settings set;
	short nMedia;
	long media[kMaxMedia][6];
};

static Handle item(DialogPtr d, short i) {
	short type;
	Handle h;
	Rect r;
	GetDialogItem(d, i, &type, &h, &r);
	return h;
}

static void setValue(DialogPtr d, short i, short v) {
	SetControlValue((ControlHandle)item(d, i), v);
}

static short getValue(DialogPtr d, short i) {
	return GetControlValue((ControlHandle)item(d, i));
}

static void setText(DialogPtr d, short i, ConstStr255Param s) {
	SetDialogItemText(item(d, i), s);
}

static long getNum(DialogPtr d, short i, long dflt) {
	Str255 s;
	long n;
	GetDialogItemText(item(d, i), s);
	if (s[0] == 0) return dflt;
	n = 0;
	for (int k=1; k<=s[0]; k++) {
		if (s[k] < '0' || s[k] > '9' || n > 99999) return dflt;
		n = n * 10 + s[k] - '0';
	}
	return n;
}

static void setNum(DialogPtr d, short i, long n) {
	Str255 s;
	s[0] = 0;
	PStrAppendNum(s, n);
	setText(d, i, s);
}

static MenuHandle popupMenu(DialogPtr d, short i) {
	ControlHandle c = (ControlHandle)item(d, i);
	PopupPrivateDataHandle p = (PopupPrivateDataHandle)(**c).contrlData;
	return (**p).mHandle;
}

static void validate(THPrint h) {
	if (Sets(h)->magic != kMagic) DefaultRecord(h);
	else Geometry(h);
}

// The printer's name and state, from the host
static void printerText(DialogPtr d, short nameItem, short statusItem) {
	struct PrintCall c;
	Str255 s;

	memset(&c, 0, sizeof c);
	c.op = kPrintInfo;
	HostCall(HostOpen(), &c);
	if (c.status != kPrintOK) {
		setText(d, nameItem, (StringPtr)"\pThe Host Printer is not connected.");
		if (statusItem) setText(d, statusItem, (StringPtr)"\p");
		return;
	}
	PStrCopyC(s, "Printer: ", 255);
	PStrAppendC(s, c.payload);
	setText(d, nameItem, s);
	if (statusItem) {
		s[0] = 0;
		if (c.b[0] == kPrinterStopped) PStrAppendC(s, "Stopped");
		else if (c.b[0] == kPrinterBusy) PStrAppendC(s, "Printing");
		else PStrAppendC(s, "Ready");
		if (c.payload[64]) {
			PStrAppendC(s, ": ");
			PStrAppendC(s, c.payload + 64);
		}
		setText(d, statusItem, s);
	}
}

static struct MyDlg *newDlg(short id, THPrint h, PItemUPP proc) {
	struct MyDlg *m = (struct MyDlg *)NewPtrClear(sizeof (struct MyDlg));
	if (!m) return NULL;
	if (!GetNewDialog(id, (Ptr)m, (WindowPtr)-1)) {
		DisposePtr((Ptr)m);
		return NULL;
	}
	m->magic = kDlgMagic;
	m->pd.pFltrProc = NULL;
	m->pd.pItemProc = proc;
	m->pd.hPrintUsr = h;
	m->set = *Sets(h);
	SetDialogDefaultItem((DialogPtr)m, 1);
	SetDialogCancelItem((DialogPtr)m, 2);
	SetDialogTracksCursor((DialogPtr)m, true);
	return m;
}

// Page Setup

static pascal void stlItem(DialogPtr d, short i) {
	struct MyDlg *m = (struct MyDlg *)d;

	switch (i) {
	case sOK: {
		struct Settings *s = Sets(m->pd.hPrintUsr);
		short sel = getValue(d, sPaper) - 1;
		*s = m->set;
		if (sel >= 0 && sel < m->nMedia) {
			s->width = m->media[sel][0];
			s->height = m->media[sel][1];
			for (int k=0; k<4; k++) s->margin[k] = m->media[sel][2+k];
			s->media = sel;
		}
		s->orient = getValue(d, sLandscape) ? 1 : 0;
		Geometry(m->pd.hPrintUsr);
		m->pd.fDoIt = true;
		m->pd.fDone = true;
		break;
	}
	case sCancel:
		m->pd.fDoIt = false;
		m->pd.fDone = true;
		break;
	case sPortrait:
	case sLandscape:
		setValue(d, sPortrait, i == sPortrait);
		setValue(d, sLandscape, i == sLandscape);
		break;
	}
}

static TPPrDlg stlInit(THPrint h) {
	struct PrintCall c;
	short ref, sel = 0;

	validate(h);
	struct MyDlg *m = newDlg(kStlDialog, h, (PItemUPP)stlItem);
	if (!m) return NULL;
	DialogPtr d = (DialogPtr)m;

	printerText(d, sPrinter, 0);

	MenuHandle menu = popupMenu(d, sPaper);
	while (CountMenuItems(menu)) DeleteMenuItem(menu, 1);

	ref = HostOpen();
	memset(&c, 0, sizeof c);
	c.op = kPrintInfo;
	HostCall(ref, &c);
	short n = c.status == kPrintOK ? c.b[1] : 0;
	if (n > kMaxMedia) n = kMaxMedia;
	for (short k=0; k<n; k++) {
		Str255 name;
		memset(&c, 0, sizeof c);
		c.op = kPrintMedia;
		c.a[0] = k;
		HostCall(ref, &c);
		if (c.status != kPrintOK) break;
		for (int j=0; j<6; j++) m->media[m->nMedia][j] = c.b[j];
		PStrCopyC(name, c.payload, 63);
		AppendMenu(menu, (StringPtr)"\px");
		SetMenuItemText(menu, m->nMedia + 1, name);
		if (c.b[0] == m->set.width && c.b[1] == m->set.height) sel = m->nMedia;
		m->nMedia++;
	}
	if (m->nMedia == 0) { // only the paper we have
		for (int j=0; j<2; j++) m->media[0][j] = j ? m->set.height : m->set.width;
		for (int j=0; j<4; j++) m->media[0][2+j] = m->set.margin[j];
		AppendMenu(menu, (StringPtr)"\pCurrent paper");
		m->nMedia = 1;
	}
	SetControlMaximum((ControlHandle)item(d, sPaper), m->nMedia);
	setValue(d, sPaper, sel + 1);

	setValue(d, sPortrait, !m->set.orient);
	setValue(d, sLandscape, m->set.orient);
	return &m->pd;
}

static pascal TPPrDlg stlInitP(THPrint h) {
	return stlInit(h);
}

// Print

static short dpiItem(short dpi) {
	return dpi <= 72 ? 1 : dpi <= 150 ? 2 : 3;
}

static pascal void jobItem(DialogPtr d, short i) {
	struct MyDlg *m = (struct MyDlg *)d;

	switch (i) {
	case jOK: {
		THPrint h = m->pd.hPrintUsr;
		long copies = getNum(d, jCopies, 1);
		if (copies < 1) copies = 1;
		if (copies > 999) copies = 999;
		short first = 1, last = iPrPgMax;
		if (getValue(d, jFrom)) {
			first = getNum(d, jFirst, 1);
			last = getNum(d, jLast, iPrPgMax);
			if (first < 1) first = 1;
			if (last < first) last = first;
		}
		struct Settings *s = Sets(h);
		short q = getValue(d, jQuality);
		s->dpi = q == 1 ? 72 : q == 2 ? 150 : 300;
		(**h).prJob.iCopies = copies;
		(**h).prJob.iFstPage = first;
		(**h).prJob.iLstPage = last;
		(**h).prJob.bJDocLoop = bDraftLoop;
		m->pd.fDoIt = true;
		m->pd.fDone = true;
		break;
	}
	case jCancel:
		m->pd.fDoIt = false;
		m->pd.fDone = true;
		break;
	case jAll:
	case jFrom:
		setValue(d, jAll, i == jAll);
		setValue(d, jFrom, i == jFrom);
		break;
	case jFirst:
	case jLast:
		setValue(d, jAll, 0);
		setValue(d, jFrom, 1);
		break;
	}
}

static TPPrDlg jobInit(THPrint h) {
	validate(h);
	struct MyDlg *m = newDlg(kJobDialog, h, (PItemUPP)jobItem);
	if (!m) return NULL;
	DialogPtr d = (DialogPtr)m;
	TPrJob job = (**h).prJob;

	printerText(d, jPrinter, jStatus);
	setNum(d, jCopies, job.iCopies > 0 ? job.iCopies : 1);
	Boolean all = job.iFstPage <= 1 && job.iLstPage >= iPrPgMax;
	setValue(d, jAll, all);
	setValue(d, jFrom, !all);
	if (!all) {
		setNum(d, jFirst, job.iFstPage);
		setNum(d, jLast, job.iLstPage);
	}
	setValue(d, jQuality, dpiItem(m->set.dpi ? m->set.dpi : kDefaultDPI));
	SelectDialogItemText(d, jCopies, 0, 32767);
	return &m->pd;
}

static pascal TPPrDlg jobInitP(THPrint h) {
	return jobInit(h);
}

static Boolean dlgMain(THPrint h, PDlgInitUPP init) {
	TPPrDlg pd = CallPDlgInitProc(init, h);
	short i;
	GrafPtr save;

	if (!pd) return false;
	DialogPtr d = (DialogPtr)pd;

	// in the middle of the main screen, a little above
	GDHandle gd = GetMainDevice();
	Rect screen = (**gd).gdRect;
	Rect r = d->portRect;
	short w = r.right - r.left, ht = r.bottom - r.top;
	MoveWindow(d, (screen.right - screen.left - w) / 2,
		(screen.bottom - screen.top - ht) / 3 + 20, false);

	GetPort(&save);
	SetPort(d);
	ShowWindow(d);
	SelectWindow(d);
	InitCursor();
	do {
		ModalDialog(pd->pFltrProc, &i);
		CallPItemProc(pd->pItemProc, d, i);
	} while (!pd->fDone);
	Boolean ok = pd->fDoIt;
	SetPort(save);

	if (((struct MyDlg *)pd)->magic == kDlgMagic) {
		((struct MyDlg *)pd)->magic = 0;
		CloseDialog(d);
		DisposePtr((Ptr)pd);
	} else {
		CloseDialog(d);
	}
	return ok;
}

// The entry points

struct HArgs {THPrint h;};
void PrintDefaultC(struct HArgs *a) {
	DefaultRecord(a->h);
}

Boolean PrValidateC(struct HArgs *a) {
	TPrint old;
	if (Sets(a->h)->magic != kMagic) {
		DefaultRecord(a->h);
		return true;
	}
	old = **a->h;
	Geometry(a->h);
	TPrint *now = *a->h;
	char *x = (char *)&old, *y = (char *)now;
	for (int k=0; k<sizeof old; k++) if (x[k] != y[k]) return true;
	return false;
}

Boolean PrStlDialogC(struct HArgs *a) {
	return dlgMain(a->h, (PDlgInitUPP)stlInitP);
}

Boolean PrJobDialogC(struct HArgs *a) {
	return dlgMain(a->h, (PDlgInitUPP)jobInitP);
}

TPPrDlg PrStlInitC(struct HArgs *a) {
	return stlInit(a->h);
}

TPPrDlg PrJobInitC(struct HArgs *a) {
	return jobInit(a->h);
}

struct DlgMainArgs {PDlgInitUPP init; THPrint h;};
Boolean PrDlgMainC(struct DlgMainArgs *a) {
	return dlgMain(a->h, a->init);
}

struct MergeArgs {THPrint dst; THPrint src;};
void PrJobMergeC(struct MergeArgs *a) {
	validate(a->src);
	validate(a->dst);
	(**a->dst).prJob = (**a->src).prJob;
	Sets(a->dst)->dpi = Sets(a->src)->dpi;
	Geometry(a->dst);
}
