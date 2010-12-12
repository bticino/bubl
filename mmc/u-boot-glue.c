/*
 * This is glue code to use the u-boot mmc driver from within the
 * bubl main.c, without changing such main.c nor davinci_mmc (from u-boot)
 */
#include <bubl/types.h>
#include <bubl/tools.h>
#include <u-boot-compat.h>
#include "part.h"
#include "mmc.h"

/* This is called from main */
int sdcard_init(void)
{
	return 0;
}

/* This is the callback from davinci_mmc */
int mmc_register(struct mmc *mmc)
{
	printk("%s\n", __func__);
	return 0;
}

/* And, finally, this is the one that reads actual blocks */
int sdcard_read_block(int blknr, unsigned long addr)
{
	return 0;
}


