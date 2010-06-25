OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH("arm")
ENTRY(_start)


SECTIONS {
	.data.header : {
		 *(.data.header);
	}
	/* Start address in EEPROM (binary) will be same as in RAM */
	. = ALIGN(0x20);
	_end_header = .;

	.text.selfcopy : {
		 *(.text.selfcopy);
	 }

	/* Code starts at 0x100 */
	. = ALIGN(0x100);
	_start = .;
	.text.boot : {
		*(.text.boot);
	}

	/* At next 0x100 multiple, put real code, after the stack */
	. = ALIGN(0x100);
	_stack_end = . - 4;
	.text : { *(.text); }

	/* data should be in the second 64k block: FIXME */
	.rodata : { *(.rodata); }
	.data : { *(.data); }
	. = ALIGN(16);
	_edata = .;
	__bss_start = .;
	.bss : { *(.bss); }
	. = ALIGN(16);
	__bss_end = .;
	_end = .;

	__size__ = _end - _end_header;

}

