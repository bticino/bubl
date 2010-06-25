#include <bubl/io.h>
#include <bubl/hw.h>
#include <bubl/hw-misc.h>
#include <bubl/tools.h>

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

void bubl_main(void)
{
	unsigned ramsize;

	misc_setup0();
	pll1_setup();
	pll2_setup();
	ddr_setup();
	misc_setup1();
	serial_setup();

	printk("Function %s (%s:%i), compile date %s\n",
	       __FILE__, __func__, __LINE__, __DATE__);

	/* Check the RAM */
	ramsize = ram_test(RAMADDR);
	printk("RAM: 0x%08x bytes (%i KiB, %i MiB)\n",
	       ramsize, ramsize >> 10, ramsize >> 20);

	/* Nothing more to do by now */
	while (1)
		;
}
