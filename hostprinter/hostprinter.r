/* Licensed under the MIT license */

/* The Host Printer: a printer driver file for the Extensions folder */

#include "Dialogs.r"
#include "Controls.r"
#include "Menus.r"
#include "Finder.r"
#include "Icons.r"
#include "MacTypes.r"

#include "icon.r"

data 'PDEF' (0, "Draft") { $$read("build/hostprinter/pdef0.bin") };
data 'PDEF' (1, "Spool") { $$read("build/hostprinter/pdef0.bin") };
data 'PDEF' (4, "Dialogs", locked) { $$read("build/hostprinter/pdef4.bin") };
data 'PDEF' (7, "General") { $$read("build/hostprinter/pdef7.bin") };
data 'PACK' (-4096, "Chooser") { $$read("build/hostprinter/pack.bin") };
data 'DRVR' (-8192, ".Print") { $$read("build/hostprinter/drvr.bin") };

resource 'STR ' (-4091) { "Printer:" };
resource 'STR ' (-8191) { "Print Job" };

/* Page Setup */
resource 'DLOG' (-8192) {
	{0, 0, 140, 420}, dBoxProc, invisible, noGoAway, 0x0, -8192, "Page Setup", noAutoCenter
};
resource 'DITL' (-8192) {{
	{108, 330, 128, 400}, Button {enabled, "OK"};
	{108, 240, 128, 310}, Button {enabled, "Cancel"};
	{40, 20, 60, 400}, Control {enabled, -8192};
	{72, 110, 90, 200}, RadioButton {enabled, "Portrait"};
	{72, 210, 90, 320}, RadioButton {enabled, "Landscape"};
	{72, 20, 90, 105}, StaticText {disabled, "Orientation:"};
	{12, 20, 30, 400}, StaticText {disabled, ""};
}};
resource 'CNTL' (-8192) {
	{40, 20, 60, 400}, 0, visible, 90, -8192, 1009, 0, "Paper:"
};
resource 'MENU' (-8192) {
	-8192, textMenuProc, allEnabled, enabled, "Paper", {
		"US Letter", noIcon, noKey, noMark, plain
	}
};

/* Print */
resource 'DLOG' (-8191) {
	{0, 0, 176, 420}, dBoxProc, invisible, noGoAway, 0x0, -8191, "Print", noAutoCenter
};
resource 'DITL' (-8191) {{
	{144, 330, 164, 400}, Button {enabled, "Print"};
	{144, 240, 164, 310}, Button {enabled, "Cancel"};
	{56, 20, 72, 80}, StaticText {disabled, "Copies:"};
	{56, 90, 72, 130}, EditText {enabled, "1"};
	{82, 20, 98, 80}, StaticText {disabled, "Pages:"};
	{80, 86, 98, 136}, RadioButton {enabled, "All"};
	{80, 140, 98, 200}, RadioButton {enabled, "From:"};
	{82, 205, 98, 245}, EditText {enabled, ""};
	{82, 255, 98, 280}, StaticText {disabled, "To:"};
	{82, 285, 98, 325}, EditText {enabled, ""};
	{108, 20, 128, 300}, Control {enabled, -8191};
	{10, 20, 26, 400}, StaticText {disabled, ""};
	{28, 20, 44, 400}, StaticText {disabled, ""};
}};
resource 'CNTL' (-8191) {
	{108, 20, 128, 300}, 0, visible, 70, -8191, 1009, 0, "Quality:"
};
resource 'MENU' (-8191) {
	-8191, textMenuProc, allEnabled, enabled, "Quality", {
		"Draft (72 dpi)", noIcon, noKey, noMark, plain;
		"Normal (150 dpi)", noIcon, noKey, noMark, plain;
		"Best (300 dpi)", noIcon, noKey, noMark, plain
	}
};

/* A job that failed: ^0 says why */
resource 'ALRT' (-8192) {
	{0, 0, 110, 380}, -8190, {
		OK, visible, sound1;
		OK, visible, sound1;
		OK, visible, sound1;
		OK, visible, sound1
	}, alertPositionMainScreen
};
resource 'DITL' (-8190) {{
	{78, 290, 98, 360}, Button {enabled, "OK"};
	{12, 70, 66, 360}, StaticText {disabled, "^0"};
	{12, 20, 44, 52}, Icon {disabled, 0};
}};

/* For the Finder */
type 'hstP' as 'STR ';
resource 'hstP' (0) { "Host Printer 1.0" };
resource 'BNDL' (128) {
	'hstP', 0, {
		'ICN#', {0, -4096};
		'FREF', {0, 128}
	}
};
resource 'FREF' (128) { 'PRER', 0, "" };
resource 'vers' (1) {
	0x01, 0x00, release, 0x00, 0, "1.0", "1.0, prints on the host's printers"
};
