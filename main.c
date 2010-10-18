#include <bubl/io.h>
#include <bubl/pll.h>
#include <bubl/hw.h>
#include <bubl/hw-misc.h>
#include <bubl/tools.h>
#include <bubl/string.h>
#include <bubl/delay.h>
#include <bubl/timer.h>
#include <bubl/adc.h>

#include <mmcsd.h>
#include <s_record.h>

#include "board.h" /* board selection using ADC values */

static unsigned ram_test(int base)
{
	u32 *bptr = (u32 *)base;
	int size;

	bptr[0] = 0xbadcaffe;
	/* start from 4M going up; max is 256M by hw */
	for (size = 1<<20; size < 1<<27; size <<=1)
		if (bptr[size] == bptr[0])
			break;
	/*
	 * So, this is the first that failed; as a 4-byte counter.
	 * This is exactly the size, so multiply by 4
	 */
	return size << 2;
}

static void bubl_work(void);
static void __attribute__((__noreturn__)) do_srecord(void);

/* This functions turns on the system, and calls the working function */
void bubl_main(void)
{
	unsigned ramsize;
	int usec1, usec2;
	int i, adcvals[6];
	struct pll_config *cfg;

	misc_setup0();
	timer_setup();
	pll_bypass();

	/* read ADC values, to identify machine types */
	adc_setup();
	for (i = 0; i < 6; i++)
		adcvals[i] = adc_read(i);
	cfg = board_get_config(adcvals);

	usec1 = nop(1000*1000);
	pll_setup(cfg);
	usec2 = nop(1000*1000);

	ddr_setup();
	misc_setup1();
	serial_setup();

	printk("Function %s (%s:%i), compile date %s\n",
	       __FILE__, __func__, __LINE__, __DATE__);

	printk("1M nops before pll: %i usec\n", usec1);
	printk("1M nops after  pll: %i usec\n", usec2);
	printk("ADC values: ");
	for (i = 0; i < 6; i++)
		printk("%i%c", adcvals[i], i==5 ? '\n' : ' ');

	/* Check the RAM */
	ramsize = ram_test(RAMADDR);
	printk("RAM: 0x%08x bytes (%i KiB, %i MiB)\n",
	       ramsize, ramsize >> 10, ramsize >> 20);

	/* Check the RAM speed */
	usec1 = mw(1000*1000, RAMADDR);
	printk("RAM speed: 1M writes in %i usec\n", usec1);

	/* Now move the stack pointer to RAM */
	asm("sub sp, %0, #16" : : "r" (RAMADDR + ramsize) : "memory");
	bubl_work();
}

/* This is the real work being done: the stack pointer is not at end-of-ram */
void __attribute__((noreturn, noinline)) bubl_work(void)
{
	unsigned long addr;
	unsigned long offset = 4096;

	/*
	 * Read u-boot to address 0x81080000 from offset 4kB.
	 * Size loaded is 256k
	 */
	const unsigned long ub_addr = 0x81080000;
	const unsigned long ub_size = 1024 * 256;
	const unsigned long step = 512; /* can't be more than 512 */

	/* make the memory area dirty, to be sure it works */
	memset((void *)ub_addr, 0xca, ub_size);


	sdcard_init();
	printk("Loading u-boot ");
	for (addr = ub_addr;
	     addr < ub_addr + ub_size;
	     addr += step, offset += step) {
		/* last argument "endian" is not used */
		MMCSD_singleBlkRead(offset, (void *)addr, step, 0);
		if ((offset & 0x1fff) == 0)
			printk(".");
	}

	/* If we have serial data or u-boot is not really there, use srecord */
	if (testc() || ((u32 *)ub_addr)[0xf] != 0xdeadbeef)
		do_srecord();

	/* jump to u-boot */
	asm("ldr pc, %0" : /* no output */ : "m" (ub_addr));

	/* Loop to prevent compiler warning about noreturn */
	while (1)
		;
}

static unsigned long load_serial(void);
void do_srecord(void)
{
	unsigned long addr;

	puts("\n\nSerial loader: waiting for s-record fields\n");
	addr = load_serial();
	if (addr != ~0) {
		printk("Load successful: jumping to 0x%08x\n", (int)addr);
		asm("ldr pc, %0" : /* no output */ : "m" (addr));
	}
	/* If successful, we won't get here */
	printk("\nLoad failed, back to work...\n");
	bubl_work();
}

/*
 * This is the "serial loader", that uses s-record fields.
 * It is the "load_serial" function in u-boot:common/cmd_load.c
 */
static unsigned long load_serial(void)
{
	char	record[SREC_MAXRECLEN + 1];
	char	binbuf[SREC_MAXBINLEN];	/* buffer for binary data	*/
	int	binlen;			/* no. of data bytes in S-Rec.	*/
	int	type;			/* return code for record type	*/
	unsigned long	addr;		/* load address from S-Record	*/
	unsigned long	size;		/* number of bytes transferred	*/
	unsigned long	store_addr;
	unsigned long	start_addr = ~0;
	unsigned long	end_addr   =  0;
	int	line_count =  0;

	while (gets(record, SREC_MAXRECLEN)) {
		type = srec_decode (record, &binlen, &addr, binbuf);
		if (type < 0)
			return (~0);	/* Invalid S-Record		*/

		switch (type) {
		case SREC_DATA2:
		case SREC_DATA3:
		case SREC_DATA4:
			store_addr = addr;
			memcpy ((char *)(store_addr), binbuf, binlen);
			if ((store_addr) < start_addr)
				start_addr = store_addr;
			if ((store_addr + binlen - 1) > end_addr)
				end_addr = store_addr + binlen - 1;
			break;
		case SREC_END2:
		case SREC_END3:
		case SREC_END4:
			udelay (10000);
			size = end_addr - start_addr + 1;
			printk ("\n"
				"## First Load Addr = 0x%08lX\n"
				"## Last  Load Addr = 0x%08lX\n"
				"## Total Size      = 0x%08lX = %ld Bytes\n",
				start_addr, end_addr, size, size
				);
			return addr;
		case SREC_START:
		default:
			break;
		}
		/* print a '.' every 100 lines */
		if ((++line_count % 100) == 0)
			putc ('.');
	}
	return ~0;
}

