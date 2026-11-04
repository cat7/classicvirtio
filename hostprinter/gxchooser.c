/* Licensed under the MIT license */

// PACK -4096 of the GX driver: GX makes the desktop printer

#include <Devices.h>
#include <Files.h>
#include <Resources.h>

#include "common.h"
#include "gx.h"

struct PackGlobals {
	Str31 driverName;
	gxJob job;
};

struct ChooserArgs {long p2; long p1; StringPtr zone; StringPtr obj; short caller; short message;};
OSErr GXChooserC(struct ChooserArgs *a, struct PackGlobals *g) {
	OSErr err = noErr;

	if (a->message == chooserInitializeMsg) {
		FCBPBRec pb;
		memset(&pb, 0, sizeof pb);
		pb.ioNamePtr = g->driverName;   // the user may have renamed the file
		pb.ioRefNum = CurResFile();
		err = PBGetFCBInfoSync(&pb);
		g->job = NULL;
		if (!err) {
			GXEnterGraphics();
			err = GXGetGraphicsError(NULL);
			if (!err) {
				err = GXInitPrinting();
				if (err) GXExitGraphics();
			}
			if (err) StopAlert(-4095, NULL);
		}
	}
	if (!err && (g->job || a->message == chooserInitializeMsg)) {
		err = GXHandleChooserMessage(&g->job, g->driverName, a->message, a->caller,
			a->obj, a->zone, (ListHandle)a->p1, a->p2);
		// terminate twice: the Chooser is closing (GX has disposed of the job)
		if (a->message == terminateMsg && a->p2 == terminateMsg) {
			GXExitPrinting();
			GXExitGraphics();
		}
	}
	return err;
}
