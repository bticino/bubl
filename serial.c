#include <bubl/types.h>
#include <bubl/io.h>

#define SERIAL_BASE ((volatile u32 *)0x01c20000)

enum ser_regs {
	SER_DATA, /* tx and rx */
	SER_IER,
	SER_IIR, /* and fifo control */
	SER_LCR,

	SER_MCR,
	SER_LSR,
	SER_UNUSED_18,
	SER_UNUSED_1C,

	SER_DLL,
	SER_DLH,
	SER_PID,
	SER_UNUSED_2C,


	SER_PWR,
	SER_MDR
};

#define TX_EMPTY	0x20

void putc(int c)
{
	volatile u32 *base = SERIAL_BASE;

	/* serial wants \r\n */
	if (c == '\n')
		putc('\r');

	/* wait for the serial to be idle (no transmission) */
	while (!(base[SER_LSR] & (TX_EMPTY)))
		;
	base[SER_DATA] = c;
}

void puts(const char *s)
{
	while (*s)
		putc(*(s++));
}

void serial_setup(void)
{
	volatile u32 *base = SERIAL_BASE;

	/* Power on */
	base[SER_PWR] = 0xe001; /* NOTE: bit 15 is undocumented */

	/* Set DLAB and write the divisor - NOTE: is DLAB used? */
	base[SER_LCR] = 0x80;
	base[SER_DLL] = 0x0d;
	base[SER_DLH] = 0x00;
	/* Clear DLAB, set 8bits per byte */
	base[SER_LCR] = 0x03;
	base[SER_MCR] = 0x00;
}
