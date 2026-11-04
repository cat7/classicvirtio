/* Licensed under the MIT license */

// LDEF -4096 of the GX driver: the Chooser's list of connections. QuickDraw
// GX fills each cell with an icon and a name (marked by two '≈'); other cells
// are plain text.

#include <Icons.h>
#include <Lists.h>
#include <Quickdraw.h>
#include <TextUtils.h>
#include <ToolUtils.h>

#include "common.h"

struct PortCell {
	char first, second;     // '≈', '≈'
	Handle iconSuite;
	Handle outputDriver, inputDriver;
	Str255 name;
};

#define HiliteModeGlobal (*(volatile unsigned char *)0x0938)

static void invert(Rect *r) {
	HiliteModeGlobal &= 0x7f;   // the highlight color
	InvertRect(r);
}

// Pascal: PROCEDURE LDEF(message: INTEGER; select: BOOLEAN; theRect: Rect;
//   theCell: Cell; dataOffset, dataLen: INTEGER; theList: ListHandle);
struct LdefArgs {ListHandle list; short len; short offset; Cell cell; Rect *r; Boolean select; char pad; short message;};
void GXLdefC(struct LdefArgs *a) {
	struct PortCell c;
	FontInfo fi;
	short len = a->len;

	if (a->message != lDrawMsg && a->message != lHiliteMsg) return;
	if (len > (short)sizeof c) len = sizeof c;
	if (len < 0) len = 0;
	memset(&c, 0, sizeof c);
	BlockMoveData(*(**a->list).cells + a->offset, &c, len);

	if (len >= 15 && (unsigned char)c.first == 0xc5 && (unsigned char)c.second == 0xc5) {
		Rect icon, label;
		short w = a->r->right - a->r->left;

		icon.top = a->r->top + 10;
		icon.left = a->r->left + w / 2 - 16;
		icon.bottom = icon.top + 32;
		icon.right = icon.left + 32;
		label = *a->r;
		label.top = icon.bottom + 2;
		EraseRect(&label);
		if (c.iconSuite) {
			PlotIconSuite(&icon, kAlignNone, a->select ? kTransformSelected : kTransformNone,
				c.iconSuite);
		}
		TextFont(applFont);
		TextSize(9);
		GetFontInfo(&fi);
		TruncString(w - 4, c.name, smTruncEnd);
		short tw = StringWidth(c.name);
		short x = a->r->left + (w - tw) / 2;
		short y = label.top + 2 + fi.ascent;
		MoveTo(x, y);
		DrawString(c.name);
		if (a->select) {
			Rect t = {y - fi.ascent - 1, x - 1, y + fi.descent + 1, x + tw + 1};
			invert(&t);
		}
		TextSize(0);
	} else {
		char *text = (char *)&c;
		EraseRect(a->r);
		GetFontInfo(&fi);
		MoveTo(a->r->left + 4, a->r->bottom - fi.descent);
		DrawText(text, 0, len);
		if (a->select) invert(a->r);
	}
}
