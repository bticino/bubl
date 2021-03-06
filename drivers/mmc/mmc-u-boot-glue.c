/*
 * This is glue code to use the u-boot mmc driver from within the
 * bubl main.c, without changing such main.c nor davinci_mmc (from u-boot)
 */
#include <bubl/types.h>
#include <bubl/tools.h>
#include <u-boot-compat.h>
#define CONFIG_SOC_DM365
#include "part.h"
#include "mmc.h"
#include "sdmmc_defs.h"

/* This is a kind of plaform data that is passed to davinci_mmc_init() */
static struct davinci_mmc mmc_sd0 = {
	.reg_base = (struct davinci_mmc_regs *)DAVINCI_MMC_SD0_BASE,
	.input_clk = 121500000,
	.host_caps = MMC_MODE_4BIT,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,
	.version = MMC_CTLR_VERSION_2,
};

/* This is called from main */
int sdmmc_init(void)
{
	bd_t unused_bd;
	struct mmc *mmc;

	mmc_initialize(&unused_bd);

	davinci_mmc_init(&unused_bd, &mmc_sd0);

	mmc = find_mmc_device(0);
	mmc_init(mmc);
	return 0;
}

/* And, finally, this is the one that reads actual blocks */
int sdmmc_read_blocks(int dev, int blk, int cnt, unsigned long addr)
{
	static struct mmc *mmc;

	/* Lazy: keep the mmc pointer static... */
	if (!mmc)
		mmc = find_mmc_device(dev);
	if (!mmc)
		return -1;
	/* call the u-boot function */
	mmc_read_blocks(mmc, (void *)addr, blk, cnt);
	return 0;
}


