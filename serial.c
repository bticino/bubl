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

/* Bits in line status register */
#define TX_EMPTY	0x20
#define RX_DR		0x01

/* output functions */
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

/* input functions */
int testc(void)
{
	volatile u32 *base = SERIAL_BASE;

	if (base[SER_LSR] & RX_DR)
		return 1;
	return 0;
}

int getc(void)
{
	volatile u32 *base = SERIAL_BASE;

	if (!testc())
		return -1;
	return base[SER_DATA];
}

char *gets(char *s, int len)
{
	int i, c;
	for (i = 0; i < len - 1; ) {
		while (!testc())
			/* wait for char */;
		c = getc();
		if (c == 0x03) { /* ctrl-C */
			puts("<INTERRUPTED>\n");
			break;
		}
		if (c == '\r') c = '\n';
		s[i++] = c;
		if (c == '\n')
			break;
	}
	s[i] = '\0';
	return s;
}


/* initial setup */
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
