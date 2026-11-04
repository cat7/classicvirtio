/* Licensed under the MIT license */

/* QuickDraw GX's own resources of the Host Printer GX driver, as data:
 * Retro68 has no templates for them */

/* a raster driver */
data 'isys' (-27648, sysheap) {
	$"7261 7374"
};

/* no connection: the pages go to the host through .VirtioPrint */
data 'comm' (-4094, sysheap) {
	$"6E6F 7073"
};

/* the Chooser's one choice, drawn as an icon */
data 'look' (-4096, sysheap) {
	$"0001 0001 0448 6F73 7400 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 0000 0000 F002 0000 0002 0C48 6F73 7420 5072 696E 7465 7200 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000"
};

/* messages: OpenConnection, CloseConnection, StartSendPage,
 * FinishSendPage, WriteData, DefaultPrinter, SetupImageData,
 * DefaultDesktopPrinter; offsets in 'pdvr' 0 */
data 'over' (-27648, sysheap) {
	$"0008 0024 0000 0004 FFFF FFFF 0025 0000 0008 FFFF FFFF 0026 0000 000C FFFF FFFF"
	$"0027 0000 0010 FFFF FFFF 0028 0000 0014 FFFF FFFF 000E 0000 001C FFFF FFFF 001E"
	$"0000 0020 FFFF FFFF 003B 0000 0024 FFFF FFFF"
};

/* raster messages: RasterDataIn */
data 'over' (-27647, sysheap) {
	$"0001 0000 0000 0018 FFFF FFFF"
};

/* 300 dpi (Best; Normal is 150), all bands, from 4 rows: the background
 * printing's memory is small (a quarter of it, less 32K), one 32-bit RGB plane */
data 'rdip' (-27648, sysheap) {
	$"0000 0008 012C 0000 012C 0000 0004 0000 0000 4000 0000 8000 0020 0001 0000 0001"
	$"0000 0000 0000 0000 0000 0001 0000 0001 0000 0001 0000 0000 0000 0000 0000 0000"
	$"0000 0001 0000 0000 FFFF FFFF FFFF 0000 0000 0000 0000 0801 0000 0000 0000 0000"
};

/* old applications: 150 dpi, raster translation */
data 'cust' (-8192, sysheap, purgeable) {
	$"0096 0096 0000 0002 0002 0021"
};

/* papers, with quarter-inch margins */
data 'ptyp' (-27648, sysheap) {
	$"0955 5320 4C65 7474 6572 0000 0000 0000 0000 0240 0000 02F4 0000 FFEE 0000 FFEE"
	$"0000 0252 0000 0306 0000 0000 0001 6873 7447 02D0 0000 0000 0000"
};

data 'ptyp' (-27647, sysheap) {
	$"0855 5320 4C65 6761 6C00 0000 0000 0000 0000 0240 0000 03CC 0000 FFEE 0000 FFEE"
	$"0000 0252 0000 03DE 0000 0000 0002 6873 7447 02C0 0000 0000 0000"
};

data 'ptyp' (-27646, sysheap) {
	$"0241 3400 0000 0000 0000 0000 022F 47AE 0325 E3D7 FFEE 0000 FFEE 0000 0241 47AE"
	$"0337 E3D7 0000 0003 6873 7447 01C0 0000 0000 0000"
};

data 'ptyp' (-27645, sysheap) {
	$"0241 3500 0000 0000 0000 0000 017F 87AE 022F 47AE FFEE 0000 FFEE 0000 0191 87AE"
	$"0241 47AE 0000 0000 6873 7447 01C0 0000 0000 0000"
};

data 'ptyp' (-27644, sysheap) {
	$"0242 3500 0000 0000 0000 0000 01CE E666 02A0 A8F6 FFEE 0000 FFEE 0000 01E0 E666"
	$"02B2 A8F6 0000 0004 6873 7447 01C0 0000 0000 0000"
};

