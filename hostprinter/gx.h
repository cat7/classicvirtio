/* Licensed under the MIT license */

// The parts of QuickDraw GX printing the Host Printer GX driver uses

#pragma once

#include <Collections.h>
#include <Lists.h>
#include <Types.h>

typedef struct OpaqueGXJob *gxJob;
typedef struct OpaqueGXFormat *gxFormat;

typedef struct {
	Fixed left, top, right, bottom;
} gxRectangle;

typedef struct {
	char *image;
	long width, height, rowBytes, pixelSize;
	long space;
	void *set, *profile;
} gxBitmap;

typedef struct {
	void *theViewPort, *theDevice, *theViewGroup, *theBitmap;
	gxBitmap theBits;
} gxOffscreenPlaneRec;

typedef struct {
	short numberOfPlanes;
	Handle offscreenStorage;
	gxOffscreenPlaneRec thePlanes[1];
} gxOffscreenRec, *gxOffscreenPtr, **gxOffscreenHdl;

typedef struct {
	long numPages, priority;
	unsigned long timeToPrint;
	long jobTimeout, firstPageToPrint;
	short jobAlert;
	Str31 appName, documentName, userName;
} gxJobInfo;

enum {
	gxPrintingTagID = -28672,
	gxRGB32Space = 0x0801,
	chooserInitializeMsg = 11,
};

// The message being handled, passed on with these parameters
OSErr GXForward0(void) = {0x7036, 0xabfb};
OSErr GXForward1(void *a) = {0x7036, 0xabfb};
OSErr GXForward2(void *a, void *b) = {0x7036, 0xabfb};
OSErr GXForward3(void *a, void *b, void *c) = {0x7036, 0xabfb};

void *GXGetInstanceContext(void) = {0x702b, 0xabfb};
void *GXSetInstanceContext(void *p) = {0x702c, 0xabfb};

pascal void GXGetFormatDimensions(gxFormat f, gxRectangle *page, gxRectangle *paper)
	= {0x203c, 0x0000, 0x0016, 0xabfe};
gxJob GXGetJob(void) = {0x203c, 0x0001, 0x0001, 0xabfe};
pascal Collection GXGetJobCollection(gxJob job) = {0x203c, 0x0000, 0x001d, 0xabfe};
pascal OSErr GetCollectionItem(Collection c, OSType tag, long id, long *size, void *data)
	= {0x7009, 0xabf6};
pascal OSErr GXInitPrinting(void) = {0x203c, 0x0000, 0x0000, 0xabfe};
pascal OSErr GXExitPrinting(void) = {0x203c, 0x0000, 0x0001, 0xabfe};
OSErr GXHandleChooserMessage(gxJob *job, StringPtr driverName, long message, long caller,
	StringPtr objName, StringPtr zoneName, ListHandle list, long p2)
	= {0x203c, 0x0002, 0x0007, 0xabfe};

void GXEnterGraphics(void) = {0x303c, 0x005f, 0xa832};
void GXExitGraphics(void) = {0x303c, 0x0060, 0xa832};
long GXGetGraphicsError(long *sticky) = {0x303c, 0x0061, 0xa832};
