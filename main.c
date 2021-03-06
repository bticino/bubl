/*
 * Copyright (C) 2011 Bticino S.p.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <bubl/io.h>
#include <bubl/pll.h>
#include <bubl/hw.h>
#include <bubl/hw-misc.h>
#include <bubl/tools.h>
#include <bubl/string.h>
#include <bubl/delay.h>
#include <bubl/timer.h>
#include <bubl/adc.h>

#include "drivers/mmc/mmc-u-boot-glue.h"
#include <s_record.h>

/*
 * board selection using normalized ADC values, see global array adcvals_n
 *
 * adcvals_n[1] : HW_VERSION
 * adcvals_n[2] : BOARD_IDENTIFICATION (LSB)
 * adcvals_n[3] : BOARD_IDENTIFICATION (MSB)
 * adcvals_n[4] : FREQ-BOOT_MODE
 * adcvals_n[5] : LCD MODEL (DINGO only!)
 *
 * Supported boards:
 * adcvals_n[3]		adcvals_n[2]	board name	status
 *	0			0	   bmx
 *	0			1	   basi		  ok
 *	0			2	   dingo	  ok
 *	0			3	   stork
 *	0			4	   owl
 *	0			5	   jumbo-1
 *	0			6	   jumbo-2
 *	0			7	   arges
 *
 */
#include "board.h"
#include "drivers/nand/nand.h"

#undef RAM_SPEED_TEST

int boot_cfg;

int sdmmc_init(void) __attribute__((weak));
int sdmmc_init(void)
{
return 0;
}
int sdmmc_read_blocks(int dev, int start_blk, int num_blks,
			int unsigned long addr) __attribute__((weak));
int sdmmc_read_blocks(int dev, int start_blk, int num_blks,
			int unsigned long addr)
{
return 0;
}

extern nand_info_t nand_info;

static unsigned ram_test(int base)
{
	u32 *bptr = (u32 *)base;
	int size;

	bptr[0] = 0xbadcaffe;
	/* start from 4M going up; max is 256M by hw */
	for (size = 1<<20; size < 1<<27; size <<= 1)
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
int adcvals_n[6];

/* This functions turns on the system, and calls the working function */
void bubl_main(void)
{
	unsigned ramsize;
	int i, adcvals[6];
	struct pll_config *pll_cfg;
	int err = 0;
	u32 *reg;

	misc_setup0();
	timer_setup();
	pll_bypass();

	/* read ADC values, to identify machine types */
	adc_setup();
	for (i = 0; i < 6; i++)
		adcvals[i] = adc_read(i);
	board_boot_cfg_get_config(adcvals, adcvals_n);

	pinmux_setup(adcvals_n[4] / 5);

	/*
	 * on BASI board turns on GIO40 - LED
	 * it on by default anyway
	 */
	if (!adcvals_n[3] && adcvals_n[2] == 1) {
		reg = (u32 *)0x01c67038;
		*reg = *reg & ~0x100;
		reg = (u32 *)0x01c6703c;
		*reg = *reg & ~0x100;
	}
	/* on DINGO board turns on GIO94 - monitor start-up timings */
	if (!adcvals_n[3] && adcvals_n[2] == 2) {
		reg = (u32 *)0x01c67060;
		*reg = 0xBFFFFFFF;
		reg = (u32 *)0x01c67068;
		*reg = 0x40000000;
	}

	pll_cfg = board_pll_get_config(adcvals_n);


	pll_setup(pll_cfg);

	ddr_setup();
	misc_setup1();
	serial_setup();

	printk("\nBUBL - %s-%s\n", __DATE__, __TIME__);

	board_dump_config(adcvals_n);

	/* Check the RAM */
	ramsize = ram_test(RAMADDR);
	printk("RAM: %i MB\n", ramsize >> 20);

	/* But also check that it is really working */
	{
		int j;
		int stepa = 0x100234;
		int stepv = 0xcacca;

		for (i = j = 0; i < ramsize; i += stepa)
			*(volatile unsigned long *)(RAMADDR + i)
				= (j += stepv);
		/* asm volatile("" : : : "memory"); */
		for (i = j = 0; i < ramsize; i += stepa)
			if (*(volatile unsigned long *)(RAMADDR + i)
			    != (j += stepv)) {
				printk("RAM err at %x)\n", RAMADDR + i);
				err = 1;
			}
	}

	if (err) {
		printk("TRAP: Restart\n");
		reset_cpu(0);
	}

#ifdef RAM_SPEED_TEST
	/* Check the RAM speed */
	usec1 = mw(1000*1000, (void *)RAMADDR);
	printk("RAM speed: 1M wr in %i usec\n", usec1);
#endif

	/* Now move the stack pointer to RAM */
	asm volatile("sub sp, %0, #16" : : "r" (RAMADDR + ramsize) : "memory");

	bubl_work();
}

/* This is the real work being done: the stack pointer is not at end-of-ram */
void __attribute__((noreturn, noinline)) bubl_work()
{
	unsigned long start_blk = 4096 / 512; /* start at 4kB within the card */

	/*
	 * Read u-boot to address 0x81080000
	 * Size loaded is 256k
	 */
	unsigned long ub_addr = 0x81080000;
	const unsigned long ub_size = 1024 * 256;
	const unsigned long blksize = 512;
	unsigned long ub_num_blks;
	int boot_from_nand;
	u32 *reg;

	ub_num_blks = ub_size / blksize;

	boot_from_nand = adcvals_n[4] / 5;
	/* make the memory area dirty, to be sure it works */
	memset((void *)ub_addr, 0xca, ub_size);

	if (!boot_from_nand) {
		if (!sdmmc_init()) {
			printk("MMC: Load %luKB offs %luKB\n",
					ub_size, start_blk / 2);
			sdmmc_read_blocks(0, start_blk, ub_num_blks, ub_addr);
		}
	} else {
		udelay(100);
		printk("NAND: ");
		if (!nand_init()) {
			/*nand_read(&nand_info, 0, 0x4a300,
			  (size_t*) &ub_num_blks, (unsigned char*) ub_addr); */
			ub_addr = 0x81100000;
			ub_addr = read_uboot_flash(&nand_info, 0,
				  (unsigned char *) ub_addr);
		}
	}

	/* If  u-boot is not really there, use srecord */
	if (((u32 *)ub_addr)[0xf] != 0xdeadbeef)
		do_srecord();
	/* If we have serial input, and it is 's', use srecord */
	if (testc() && getc() == 's')
		do_srecord();

	/* on BASI turn off GIO40 - LED */
	if (!adcvals_n[3] && adcvals_n[2] == 1) {
		reg = 0x01c6703C;
		*reg = *reg | 0x100;
	}
	/* on DINGO turn off GIO94 - monitor start-up timings */
	if (!adcvals_n[3] && adcvals_n[2] == 2) {
		reg = (u32 *)0x01c6706C;
		*reg = 0x40000000;
	}

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

	puts("\n\nSerial loader: s-record\n");
	addr = load_serial();
	if (addr != ~0) {
		printk("Load OK: jumping to 0x%08x\n", (int)addr);
		asm("ldr pc, %0" : /* no output */ : "m" (addr));
	}
	/* If successful, we won't get here */
	printk("\nLoad KO\n");
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
		type = srec_decode(record, &binlen, &addr, binbuf);
		if (type < 0)
			return ~0;	/* Invalid S-Record		*/

		switch (type) {
		case SREC_DATA2:
		case SREC_DATA3:
		case SREC_DATA4:
			store_addr = addr;
			memcpy((char *)(store_addr), binbuf, binlen);
			if ((store_addr) < start_addr)
				start_addr = store_addr;
			if ((store_addr + binlen - 1) > end_addr)
				end_addr = store_addr + binlen - 1;
			break;
		case SREC_END2:
		case SREC_END3:
		case SREC_END4:
			udelay(10000);
			size = end_addr - start_addr + 1;
			printk("\n"
				"# First Ld Addr = 0x%08lX\n"
				"# Last  Ld Addr = 0x%08lX\n"
				"# Total Size    = 0x%08lX = %ld Bytes\n",
				start_addr, end_addr, size, size
				);
			return addr;
		case SREC_START:
		default:
			break;
		}
		/* print a '.' every 100 lines */
		if ((++line_count % 100) == 0)
			putc('.');
	}
	return ~0;
}

