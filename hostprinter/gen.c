/* Licensed under the MIT license */

// PDEF 7: PrGeneral

#include "common.h"

static const short resolutions[] = {72, 144, 150, 300};

struct GenArgs {Ptr data;};
void PrGeneralC(struct GenArgs *a) {
	TGnlData *g = (TGnlData *)a->data;

	g->iError = noErr;
	switch (g->iOpCode) {
	case getRslDataOp: {
		TGetRslBlk *b = (TGetRslBlk *)g;
		b->iRgType = RgType1;
		b->xRslRg.iMin = b->yRslRg.iMin = 72;
		b->xRslRg.iMax = b->yRslRg.iMax = 300;
		b->iRslRecCnt = sizeof resolutions / sizeof *resolutions;
		for (int i=0; i<b->iRslRecCnt; i++) {
			b->rgRslRec[i].iXRsl = resolutions[i];
			b->rgRslRec[i].iYRsl = resolutions[i];
		}
		break;
	}
	case setRslOp: {
		TSetRslBlk *b = (TSetRslBlk *)g;
		Boolean ok = false;
		for (int i=0; i<sizeof resolutions / sizeof *resolutions; i++) {
			if (b->iXRsl == resolutions[i] && b->iYRsl == resolutions[i]) ok = true;
		}
		if (!ok) {
			g->iError = NoSuchRsl;
			break;
		}
		if (Sets(b->hPrint)->magic != kMagic) DefaultRecord(b->hPrint);
		Sets(b->hPrint)->res = b->iXRsl;
		Geometry(b->hPrint);
		break;
	}
	case getRotnOp: {
		TGetRotnBlk *b = (TGetRotnBlk *)g;
		b->fLandscape = Sets(b->hPrint)->magic == kMagic && Sets(b->hPrint)->orient;
		break;
	}
	case draftBitsOp:
	case noDraftBitsOp:
		break;
	default:
		g->iError = OpNotImpl;
		break;
	}
}
