Virtio for Classic
==================

QEMU is a good way to experiment with the "Classic" Mac OS.
The Virtio devices make it even more fun to use!

Building
========

The [Retro68](https://github.com/autc04/Retro68) compilers are used.
(*Make sure that your checkout is from 2023-12-16 or later --
there was an important bugfix.*)

Update your $PATH to include the Retro68 toolchain (powerpc-apple-macos-gcc etc).

Then:

	git clone https://github.com/elliotnunn/classicvirtio.git
	cd classicvirtio
	make

# Command-line args for qemu-system-ppc

	-device loader,addr=0x4000000,file=/PATH/TO/classicvirtio/build/ndrv/ndrvloader
	-prom-env "boot-command=init-program go"

# Command-line args for qemu-system-m68k

	-device nubus-virtio-mmio,romfile=/PATH/TO/classicvirtio/build/classic/declrom

On the LC 475 of https://github.com/cat7/m68k-rom (q700 branch), whose
only slot is the processor-direct slot, one card carries both the
Ethernet and the virtio devices:

	-device mac-pds-ethernet-virtio,netdev=net0,romfile=/PATH/TO/classicvirtio/build/classic/declrom-pds

Tablet input
============

*Lets the mouse cursor move seamlessly in and out of the virtual machine window*

- bug: scroll wheel support unreliable, and on Mac OS 9 only

**PowerPC: works out of the box**

	-device virtio-tablet-pci

**68k: requires virtio-mmio patch to QEMU**

	-device virtio-tablet-device

Disk driver
===========

*Lets QEMU use unpartitioned disk images, like Basilisk II or Mini vMac*

- bug: read-only
- bug: freezes on PowerPC

**PowerPC: not working yet** **68k: works out of the box**

	-blockdev driver=file,read-only=on,node-name=FOO,filename=DISK.IMG
	-device virtio-blk,drive=FOO

9P device
=========

*Presents a folder on the host computer as a bootable hard drive on the guest computer*

- resource forks in \*.rdump and type/creator codes in \*.idump
- append `_1` to mount_tag to use the native fork format on a macOS host (needs patches)
- bug: some filesystem operations (e.g. CatMove) unimplemented
- bug: booting qemu-system-m68k requires hacks to PRAM

**PowerPC: works out of the box**

	-device virtio-9p-pci,fsdev=UNIQUENAME,mount_tag="Macintosh HD"
	-fsdev local,id=UNIQUENAME,security_model=none,path=/PATH/TO/HOST/FOLDER
	# Use this option to boot from the device:
	-device loader,addr=0x4400000,file="/PATH/TO/HOST/FOLDER/System Folder/Mac OS ROM"

**68k: requires virtio-mmio patch to QEMU**

	-device virtio-9p-device,fsdev=UNIQUENAME,mount_tag="Macintosh HD"
	-fsdev local,id=UNIQUENAME,security_model=none,path=/PATH/TO/HOST/FOLDER

Host printer
============

*Prints on the host's printers, or saves PDFs in a host folder*

Put the "Host Printer" file (`build/classic/HostPrinter.bin`, MacBinary) in
the Extensions folder and choose "Host Printer" in the Chooser. Pages are
drawn on the guest at 72, 150 or 300 dpi and sent to the host as pixels;
QEMU makes them into a PDF and prints it with CUPS. Page Setup lists the
host printer's papers and margins.

With QuickDraw GX on, the Chooser shows only GX drivers: use the "Host
Printer GX" file (`build/classic/HostPrinterGX.bin`) instead, choose it in
the Chooser and click Create to make its desktop printer. GX draws the pages
at 150 dpi, or 300 dpi with Quality: Best in the Print dialog. The host
printer's papers that GX lacks become paper type files in the Extensions
folder when the desktop printer is made; jobs default to manual paper feed,
which keeps GX from asking for papers its tray does not have.

**68k: requires QEMU with virtio-print-device (cat7/m68k-rom q700 branch)**

	-device virtio-print-device                       # the default printer
	-device virtio-print-device,printer=QUEUE_NAME    # another (lpstat -e)
	-device virtio-print-device,save-to=/PATH/TO/FOLDER

**PowerPC: not yet**

GPU
===

*Removed in favour of SolraBizna's QFB work in progress*

*Might be brought back in future as a 3D coprocessor, but not a display device*

Debug output
============

Append the following to your QEMU command line:

	-device virtio-serial -device virtconsole,chardev=foo -chardev stdio,id=foo
