/*
 * This builds the header for UBL, to be placed in SPI flash.
 */
#include <bubl/types.h>
#include <bubl/symbols.h>

struct ubl_header __attribute__((__section__(".data.header")))
ubl_header = {
	.magic		= 0xa1aced00,
	.entry_point	= _start,
	.size		= (int)__size__,
	.params		= 0x0100,
	.startaddr	= _end_header,
	.loadaddr	= _end_header,
};
