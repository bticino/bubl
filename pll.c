#include <bubl/types.h>
#include <bubl/tools.h>
#include <bubl/pll.h>
#include <bubl/delay.h>
#include <bubl/hw.h>
/*
 * PLL setup for DM365. Originally from "device.c", by Sandeep Paulraj,
 * found in the dm3xx_sd_boot-6.1 package. Now rewritten and cleaned up.
 */

int pll_bypass(void) /* This function added by ARub, as jtag has no reset */
{
	/* Put both devices in bypass mode */
	pll1_base[PLL_PLLCTL] = 0;
	pll2_base[PLL_PLLCTL] = 0;
	trivial_loop(150);
	pll1_base[PLL_PLLCTL] = 8; /* reset */
	pll2_base[PLL_PLLCTL] = 8; /* reset */
	return 0;
}

/*
 * The two plls are the same hardware cell, so factorize code where possible
 */

static void __pll_reset1(volatile u32 *base)
{
	base[PLL_PLLCTL] &= ~(1 << 1); /* not power down */
	/* Note: original code cleared bit 8 in PLLCTL, but it's unused */
	base[PLL_PLLCTL] &= ~(1 << 5);	/* enable PLLEN as source */
	base[PLL_PLLCTL] &= ~(1 << 0);	/* and then PLLEN itself -> bypass */
	udelay(2); 			/* "4 reference clock cycles" */
	base[PLL_PLLCTL] |=  (1 << 3);	/* Reset */
	udelay(10);			/* "at least 5 microseconds" */
	base[PLL_PLLCTL] &= ~(1 << 3);	/* Out of reset */
}

static void __pll_sequence(volatile u32 *base)
{
	/*
	 * This is the sequence to enable the new mult and div values
	 * The manual has TENABLE, TENABLEDIV, TINITZ in this order,
	 * but bits are TENABLEDIV (18), TENABLE (17), TINITZ (16).
	 */
						  /* ENABLE ENABLEDIV INITZ */
	base[PLL_SECCTL] = (1 << 22) | (7 << 16); /*  1        1        1   */
	base[PLL_SECCTL] = (1 << 22) | (6 << 16); /*  1        1        0   */
	base[PLL_SECCTL] = (1 << 22) | (0 << 16); /*  0        0        0   */
	base[PLL_SECCTL] = (1 << 22) | (1 << 16); /*  1        1        1   */
}

static void __pll_wait_lock(volatile u32 *base)
{
	/* This is in the system controller, unfortunately */
	volatile u32 *addr;

	if (base == pll1_base)
		addr = &SYSTEM->PLL0_CONFIG;
	else
		addr = &SYSTEM->PLL1_CONFIG;

	while ( (*addr & (7 << 24)) != (7 << 24))
		;
}


static void __pll_setup_one(volatile u32 *base, struct pll_config_one *cfg)
{
	struct pll_postdivs *div;
	int i;

	__pll_reset1(base);

	base[PLL_PLLM] = cfg->pllm;
	/* Note: b15 undocumented: was in original code, keep for symmetry */
	base[PLL_PREDIV] = cfg->prediv | (1 << 15);
	/* b15 is the enable bit */
	base[PLL_POSTDIV] = cfg->postdiv | (1 << 15);

	__pll_sequence(base);

	/* Individual dividers */
	for (i = 0, div = cfg->divs; i < cfg->ndivs; i++, div++)
		base[div->addr] = (1 << 15) | div->value;

	trivial_loop(300); /* FIXME */

	/* Write the align register: all of them, but tell pll1 from pll2 */
	if (cfg->ndivs > 4)
		i = 0x1ff; /* pll1 */
	else
		i = 0x00f; /* pll2 */
	base[PLL_ALNCTL] = i;

	base[PLL_PLLCMD] = 0x00000001;  /* GOSET */
	while (base[PLL_PLLSTAT] & 1)
		/* wait for gostat to clear */;

	/* Wait fot the PLL to lock (FIXME: should it be _before_ the go? */
	__pll_wait_lock(base);

	/* enable it */
	base[PLL_PLLCTL] |= (1 << 0);
}

int pll_setup(struct pll_config *cfg)
{
	__pll_setup_one(pll1_base, &cfg->pll1);
	__pll_setup_one(pll2_base, &cfg->pll2);

	//do this after PLL's have been set up
	SYSTEM->PERI_CLKCTRL = cfg->clkctrl;
	return 0;
}

