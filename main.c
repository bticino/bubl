#include <bubl/io.h>
#include <bubl/hw.h>
#include <bubl/hw-misc.h>
#include <bubl/tools.h>
#include <bubl/string.h>
#include <bubl/delay.h>
#include <bubl/timer.h>

#include <mmcsd.h>

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

/* This functions turns on the system, and calls the working function */
void bubl_main(void)
{
	unsigned ramsize;
	int usec1, usec2;

	timer_setup();
	usec1 = nop(100*1000);
	misc_setup0();
	pll1_setup();
	pll2_setup();
	ddr_setup();
	misc_setup1();
	serial_setup();
	usec2 = nop(1000*1000);

	printk("Function %s (%s:%i), compile date %s\n",
	       __FILE__, __func__, __LINE__, __DATE__);

	printk("==> usec1 %i\n==> usec2 %i\n", usec1, usec2);

	/* Check the RAM */
	ramsize = ram_test(RAMADDR);
	printk("RAM: 0x%08x bytes (%i KiB, %i MiB)\n",
	       ramsize, ramsize >> 10, ramsize >> 20);

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
	printk("\nJumping to u-boot...\n");

	/* jump to u-boot */
	asm("ldr pc, %0" : /* no output */ : "m" (ub_addr));

	/* Loop to prevent compiler warning about noreturn */
	while (1)
		;
}
