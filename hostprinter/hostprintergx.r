/* Licensed under the MIT license */

/* The Host Printer GX: a QuickDraw GX printer driver for the Extensions
 * folder, printing through the .VirtioPrint driver of the declaration ROM */

#include "MacTypes.r"
#include "Finder.r"
#include "Icons.r"
#include "Dialogs.r"
#include "Controls.r"
#include "Menus.r"

#include "gxres.r"

data 'pdvr' (0, "HostPrinterGX", sysheap, purgeable) { $$read("build/hostprinter/gxpdvr.bin") };
data 'PACK' (-4096, "PrintingPACK", purgeable) { $$read("build/hostprinter/gxpack.bin") };
data 'LDEF' (-4096, "LDEF", purgeable) { $$read("build/hostprinter/gxldef.bin") };

/* The GX Chooser: a "Create" button and a pop-up of the 'look' choices */
resource 'STR ' (-4096, sysheap, purgeable) { "XXXXXXXXXXXXXXXXXXXXXX" };
resource 'STR ' (-4091, sysheap, purgeable) { "" };
resource 'STR ' (-4093, sysheap, purgeable) { "Create" };
data 'GNRL' (-4096, sysheap, purgeable) { $"0B05" };
/* left button, right, on (where the pop-up goes), off, label */
data 'nrct' (-4096, sysheap, purgeable) {
	$"0005 0070 00FB 0084 0137 0000 0000 0000 0000 FF9B 00B4 FFAF 0190"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
};
resource 'MENU' (-4096, sysheap, purgeable) {
	-4096, textMenuProc, allEnabled, enabled, "", {}
};
resource 'CNTL' (-4096, sysheap, purgeable) {
	{0, 0, 20, 213}, 0, visible, 95, -4096, 1009, 0, "Connect via:"
};
resource 'DITL' (-4096, sysheap, purgeable) {{
	{3, 238, 23, 451}, Control {enabled, -4096};
}};
resource 'ALRT' (-4095, sysheap, purgeable) {
	{30, 30, 205, 410}, -4095, {
		OK, visible, sound1; OK, visible, sound1;
		OK, visible, sound1; OK, visible, sound1
	}, alertPositionParentWindowScreen
};
resource 'DITL' (-4095, sysheap, purgeable) {{
	{143, 310, 163, 368}, Button {enabled, "OK"};
	{23, 87, 119, 377}, StaticText {disabled,
		"The Host Printer GX could not be used. Quitting one or more "
		"applications and then trying again may help."};
}};

/* For the Finder: the driver, and the desktop printers made with it */
resource 'ICN#' (-27646, sysheap, purgeable) {{
	$"00000000 00000000 003FFC00 00200400"
	$"002FC400 00200400 002FE400 00200400"
	$"0FFFFFF0 10000008 20000004 20000004"
	$"20000074 20000074 20000004 3FFFFFFC"
	$"20000004 23FFFFC4 22000044 22000044"
	$"3C00003C 02000040 027E0040 02000040"
	$"027FC040 02000040 03FFFFC0 00000000"
	$"00000000 00000000 00000000 00000000",
	$"00000000 00000000 003FFC00 003FFC00"
	$"003FFC00 003FFC00 003FFC00 003FFC00"
	$"0FFFFFF0 1FFFFFF8 3FFFFFFC 3FFFFFFC"
	$"3FFFFFFC 3FFFFFFC 3FFFFFFC 3FFFFFFC"
	$"3FFFFFFC 3FFFFFFC 3FFFFFFC 3FFFFFFC"
	$"3FFFFFFC 03FFFFC0 03FFFFC0 03FFFFC0"
	$"03FFFFC0 03FFFFC0 03FFFFC0 00000000"
	$"00000000 00000000 00000000 00000000"
}};
resource 'ICN#' (-27645, sysheap, purgeable) {{
	$"00000000 00000000 003FFC00 00200400"
	$"002FC400 00200400 002FE400 00200400"
	$"0FFFFFF0 10000008 20000004 20000004"
	$"20000074 20000074 20000004 3FFFFFFC"
	$"20000004 23FFFFC4 22000044 22000044"
	$"3C00003C 02000040 027E0040 02000040"
	$"027FC040 02000040 03FFFFC0 00000000"
	$"00000000 00000000 00000000 00000000",
	$"00000000 00000000 003FFC00 003FFC00"
	$"003FFC00 003FFC00 003FFC00 003FFC00"
	$"0FFFFFF0 1FFFFFF8 3FFFFFFC 3FFFFFFC"
	$"3FFFFFFC 3FFFFFFC 3FFFFFFC 3FFFFFFC"
	$"3FFFFFFC 3FFFFFFC 3FFFFFFC 3FFFFFFC"
	$"3FFFFFFC 03FFFFC0 03FFFFC0 03FFFFC0"
	$"03FFFFC0 03FFFFC0 03FFFFC0 00000000"
	$"00000000 00000000 00000000 00000000"
}};
type 'hstG' as 'STR ';
resource 'hstG' (0) { "Host Printer GX 1.0" };
resource 'BNDL' (-27647, sysheap, purgeable) {
	'hstG', 0, {
		'ICN#', {0, -27646; 1, -27645};
		'FREF', {0, -27647; 1, -27646; 2, -27645; 3, -27644; 4, -27643; 5, -27642}
	}
};
resource 'FREF' (-27647, sysheap, purgeable) { 'pdvr', 0, "" };
resource 'FREF' (-27646, sysheap, purgeable) { 'dpnn', 1, "" };
resource 'FREF' (-27645, sysheap, purgeable) { 'dpcn', 1, "" };
resource 'FREF' (-27644, sysheap, purgeable) { 'dpna', 1, "" };
resource 'FREF' (-27643, sysheap, purgeable) { 'dvcf', 1, "" };
resource 'FREF' (-27642, sysheap, purgeable) { 'dppz', 1, "" };
resource 'vers' (-27648, sysheap, purgeable) {
	0x01, 0x00, release, 0x00, 0, "1.0", "Host Printer GX"
};
resource 'vers' (1, sysheap, purgeable) {
	0x01, 0x00, release, 0x00, 0, "1.0", "1.0, prints on the host's printers"
};
