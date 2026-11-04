/* Licensed under the MIT license */

// PACK -4096: what the Chooser shows for the Host Printer

#include <Devices.h>
#include <Lists.h>

#include "common.h"

// Pascal: FUNCTION Device(message, caller: INTEGER; objName, zoneName: StringPtr;
//                         p1, p2: LONGINT): OSErr;
struct ChooserArgs {long p2; long p1; StringPtr zone; StringPtr obj; short caller; short message;};
OSErr ChooserC(struct ChooserArgs *a) {
	ListHandle list = (ListHandle)a->p1;
	Cell cell = {0, 0};

	switch (a->message) {
	case fillListMsg: {
		struct PrintCall c;
		Str255 name;

		memset(&c, 0, sizeof c);
		c.op = kPrintInfo;
		HostCall(HostOpen(), &c);
		if (c.status == kPrintOK) {
			PStrCopyC(name, c.payload, 63);
		} else {
			PStrCopyC(name, "Not connected", 255);
		}
		LAddRow(1, 0, list);
		LSetCell(name + 1, name[0], cell, list);
		break;
	}
	case getSelMsg:
		LSetSelect(true, cell, list);
		break;
	}
	return noErr;
}
