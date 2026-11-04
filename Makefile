# Copyright (c) 2023 Elliot Nunn
# Licensed under the MIT license

# Build the drivers (PPC and 68k) with:
#     PATH=Retro68-build/toolchain/bin make
all: classic ndrv build/test
classic: build/classic/declrom build/classic/declrom-pds build/classic/HostPrinter.bin build/classic/HostPrinterGX.bin
ndrv: build/ndrv/ndrvloader

# Tell make not to delete the intermediate files
# (helpful when debugging this ususual build process)
.SECONDARY:

# Create subdirectories for the build to go into
$(shell mkdir -p build/classic build/ndrv)

# The supported Virtio devices for each Mac platform (see device-9p.c etc)
#     "CLASSIC" means a 68k DRVR for a NuBus device under qemu-system-m68k
#     "NDRV" means a PowerPC NDRV for a PCI device under qemu-system-ppc
DEVICES_CLASSIC = block 9p input print
DEVICES_NDRV = block 9p input

# And these are the C files that each device-*.c depends on (some are arch-specific)
SUPPORT := $(filter-out device-%.c,$(wildcard *.c))
SUPPORT := $(filter-out standalone-%.c,$(SUPPORT)) # special files
SUPPORT := $(filter-out ndrvloader.c,$(SUPPORT)) # special file
SUPPORT_CLASSIC = $(filter-out %-ndrv.c,$(SUPPORT))
SUPPORT_NDRV = $(filter-out %-classic.c,$(SUPPORT))

# Settle a dispute between MacTypes.h and stdbool.h
CDEFS = -DTYPE_BOOL -Dbool=_Bool -Dtrue=1 -Dfalse=0 -Wno-scalar-storage-order -Wall

# Uncomment this to generate a CPU time profile for the 9p driver
# To view it, run "./9profile.sh | flamegraph.pl > fg.svg"
# INSTRUMENT = -DINSTRUMENT=1 -finstrument-functions

############################# CLASSIC DRVR #############################

INTERFACEONLY = $(shell m68k-apple-macos-gcc -print-file-name=libInterface.a)
ALLLIBS = $(shell for x in libInterface.a libgcov.a libg.a libm.a libstdc++.a libgcc.a libc.a; do m68k-apple-macos-gcc -print-file-name=$$x; done)

# The classic drivers are linked together into one "card declaration ROM":
#     qemu-system-m68k -device nubus-virtio-mmio,romfile=build/classic/declrom
# This ROM incorporates the actual DRVRs and a few standalone C functions.
build/classic/declrom: declrom.s \
		$(patsubst %,build/classic/drvr-%.elf,$(DEVICES_CLASSIC)) \
		$(patsubst standalone-%.c,build/classic/standalone-%,$(wildcard standalone-*.c))
	m68k-apple-macos-as -o build/classic/declrom.o declrom.s
	m68k-apple-macos-ld -o build/classic/declrom.elf build/classic/declrom.o
	m68k-apple-macos-objcopy -O binary -j .text build/classic/declrom.elf $@
	python3 scripts/calculatecrc.py $@

# The same for the LC processor-direct slot card with Ethernet:
#     qemu-system-m68k -M lc475 -device mac-pds-ethernet-virtio,romfile=build/classic/declrom-pds
build/classic/declrom-pds: declrom.s \
		$(patsubst %,build/classic/drvr-%.elf,$(DEVICES_CLASSIC)) \
		$(patsubst standalone-%.c,build/classic/standalone-%,$(wildcard standalone-*.c))
	m68k-apple-macos-as --defsym PDS_ETHERNET=1 -o build/classic/declrom-pds.o declrom.s
	m68k-apple-macos-ld -o build/classic/declrom-pds.elf build/classic/declrom-pds.o
	m68k-apple-macos-objcopy -O binary -j .text build/classic/declrom-pds.elf $@
	python3 scripts/calculatecrc.py $@

# The standalone functions built using different compiler options (PC-relative addressing FYI)
build/classic/standalone-%: standalone-%.c standalone68k.lds
	m68k-apple-macos-gcc $(CDEFS) -c -m68020 -O0 -mpcrel -o $@.o $<
	m68k-apple-macos-ld --no-warn-rwx-segments -e 0 --script standalone68k.lds -o $@.elf $@.o $(ALLLIBS)
	m68k-apple-macos-objcopy -O binary -j .text $@.elf $@

# Compile the DRVR code files with an A5 global model
build/classic/%.o: %.c
	m68k-apple-macos-gcc $(CDEFS) $(INSTRUMENT) -c -m68020 -Os -msep-data -ffunction-sections -fdata-sections -o $@ $<

# Link each driver into an ELF, which will be included in the declaration ROM
# This code uses A5-refs (-msep-data) so we cannot use the Retro68 libc, but Interfaces are fine
build/classic/drvr-%.elf: drvr.lds build/classic/jumpglue.o build/classic/device-%.o $(patsubst %.c,build/classic/%.o,$(SUPPORT_CLASSIC)) $(wildcard  a5libs/*.a)
	m68k-apple-macos-ld.real -Ttext-segment 0xcd790000 -e asmOpen --gc-sections -o $@ --script $^ $(INTERFACEONLY)

# The DRVR has some assembly code to call through to C
build/classic/%.o: %.s
	m68k-apple-macos-as -o $@ $^

########################### HOST PRINTER FILE ###########################

# A printer driver for the Extensions folder ("Host Printer" in the Chooser)
# that prints through the .VirtioPrint driver of the declaration ROM.
# Each code resource is linked at two addresses: a difference means code
# that is not position-independent.
$(shell mkdir -p build/hostprinter)
HP_CFLAGS = $(CDEFS) -c -m68020 -Os -mpcrel -ffunction-sections -fdata-sections \
	-fno-builtin -fno-tree-loop-distribute-patterns
HP_LD = m68k-apple-macos-ld --no-warn-rwx-segments -e _start --gc-sections --script hostprinter/code.lds

build/hostprinter/%.o: hostprinter/%.c hostprinter/common.h hostprinter/gx.h printcall.h
	m68k-apple-macos-gcc $(HP_CFLAGS) -o $@ $<

build/hostprinter/%-entry.o: hostprinter/%.s hostprinter/entry.inc
	m68k-apple-macos-as -I hostprinter -o $@ $<

build/hostprinter/%.bin: build/hostprinter/%.elf
	m68k-apple-macos-objcopy -O binary -j .text $< $@
	m68k-apple-macos-objcopy -O binary -j .text $(<:.elf=.elf2) $@.2
	cmp $@ $@.2
	m68k-apple-macos-objcopy -O binary -j .data $< $@.data
	test ! -s $@.data

define HP_CODE
build/hostprinter/$(1).elf: build/hostprinter/$(1)-entry.o $(patsubst %,build/hostprinter/%.o,$(2)) hostprinter/code.lds
	$$(HP_LD) -o $$@ build/hostprinter/$(1)-entry.o $(patsubst %,build/hostprinter/%.o,$(2))
	$$(HP_LD) --defsym BASE=0x12340 -o $$(@:.elf=.elf2) build/hostprinter/$(1)-entry.o $(patsubst %,build/hostprinter/%.o,$(2))
endef
$(eval $(call HP_CODE,pdef0,doc common))
$(eval $(call HP_CODE,pdef4,dlg common))
$(eval $(call HP_CODE,pdef7,gen common))
$(eval $(call HP_CODE,pack,chooser common))
$(eval $(call HP_CODE,drvr,))
$(eval $(call HP_CODE,gxpdvr,gxdriver common))
$(eval $(call HP_CODE,gxpack,gxchooser common))
$(eval $(call HP_CODE,gxldef,gxldef common))

build/classic/HostPrinter.bin: hostprinter/hostprinter.r hostprinter/icon.r \
		$(patsubst %,build/hostprinter/%.bin,pdef0 pdef4 pdef7 pack drvr)
	Rez -I hostprinter -I $(dir $(shell which Rez))../RIncludes -o "build/classic/Host Printer.bin" -t PRER -c hstP hostprinter/hostprinter.r
	cp "build/classic/Host Printer.bin" $@

# The same for QuickDraw GX, which shows only GX drivers in the Chooser
build/classic/HostPrinterGX.bin: hostprinter/hostprintergx.r hostprinter/gxres.r hostprinter/icon.r \
		$(patsubst %,build/hostprinter/%.bin,gxpdvr gxpack gxldef)
	Rez -I hostprinter -I $(dir $(shell which Rez))../RIncludes -o "build/classic/Host Printer GX.bin" -t pdvr -c hstG hostprinter/hostprintergx.r
	cp "build/classic/Host Printer GX.bin" $@

############################# POWERPC NDRV #############################

# Open Firmware compatible program to load our drivers into the device tree
# (These section addresses match the Mac OS Trampoline loader, so they're safe.)
build/ndrv/ndrvloader: ndrvloader.s ndrvloader.c build/ndrv/allndrv
	powerpc-apple-macos-gcc $(CDEFS) -Os -e entrytvec -Wl,--section-start=.data=0x100000 -Wl,--section-start=.text=0x200000 -o $@ ndrvloader.s ndrvloader.c

# Glom all the NDRVs together: the loader shim can unpick them
build/ndrv/allndrv: $(patsubst %,build/ndrv/ndrv-%,$(DEVICES_NDRV))
	cat $^ >$@

build/ndrv/ndrv-%.so: device-%.c $(SUPPORT_NDRV) ndrv.exp ndrv.lds
	powerpc-apple-macos-gcc -o $@ \
		$(CDEFS) \
		-Os -ffunction-sections -fdata-sections \
		-T ndrv.lds -Wl,-bE:ndrv.exp -Wl,--gc-sections -Wl,--gc-keep-exported \
		$< $(SUPPORT_NDRV) \
		-lStdCLib -lDriverServicesLib -lNameRegistryLib -lPCILib -lVideoServicesLib -lDriverLoaderLib -lInterfaceLib -lControlsLib

build/ndrv/ndrv-%: build/ndrv/ndrv-%.so
	MakePEF -o $@ $^

############################### TEST APP ###############################

build/test: $(wildcard test/*.c)
	m68k-apple-macos-gcc $(CDEFS) -o $@.dsk $^
	DumpHFS $@.dsk build/ || echo temporary hack pending implementation of AppleDouble
