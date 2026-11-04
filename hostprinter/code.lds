/* Licensed under the MIT license */

/* A code resource: position-independent (-mpcrel), entry first,
 * no writable globals. Linked at BASE (0 unless defined) */
SECTIONS {
	. = DEFINED(BASE) ? BASE : 0;
	.text : {
		*(.text.entry);
		*(.text .text.*);
		*(.rodata .rodata.*);
		. = ALIGN(2);
	}
	.data : { *(.data .data.* .sdata .sdata.* .bss .bss.* COMMON) }
	/DISCARD/ : { *(.comment) }
}
