#include <bubl/pll.h>
#include <bubl/tools.h>
#include "board.h"
#include "const.h" /* board selection using ADC values */

int normalize_adc(int adcval)
{
	if (adcval <= 43)
		return 0;
	if (adcval <= 128)
		return 1;
	if (adcval <= 213)
		return 2;
	if (adcval <= 313)
		return 3;
	if (adcval <= 441)
		return 4;
	if (adcval <= 580)
		return 5;
	if (adcval <= 717)
		return 6;
	if (adcval <= 828)
		return 7;
	if (adcval <= 947)
		return 8;
	return 9;
}

/*
 * PLL configuration for board with 297Mhz cpu freq
 * DDR2 and other peripherals frequencies depend on the cpu
 * frequency
 */

/* Input here is 486MHz */
static struct pll_postdivs Mhz297_pll1_divs[] = {
	{ PLL_PLLDIV2,  1},	/* 486/2 = 243 -> HDVICP */
	{ PLL_PLLDIV3,  1},	/* 486/2 = 243 -> MJCP,HDVICP */
	{ PLL_PLLDIV4,  3},	/* 486/4 = 121.5 -> EDMA and periph */
	{ PLL_PLLDIV5,  1},	/* 486/2 = 243 -> VPSS */
	{ PLL_PLLDIV6, 17},	/* 486/18 = 27 -> VENV */
	{ PLL_PLLDIV7,  0},	/* 486 -> DDR (post divided by 2 = 243 */
	{ PLL_PLLDIV8,  3},	/* 486/4 = 121.5 -> MMC0 */
	{ PLL_PLLDIV9,  1},	/* 486/2 = 243 -> CLKOUT */
};

/* Input here is 594 MHz */
static struct pll_postdivs Mhz297_pll2_divs[] = {
	{ PLL_PLLDIV2,  1},	/* 594/2 = 297 -> ARM */
	{ PLL_PLLDIV4, 28},	/* 594/29 = 20.4827 -> VOICE */
	{ PLL_PLLDIV5,  7},	/* 594/8 = 74.25 -> VIDEO */
};

static struct pll_config Mhz297_pll_config = {
	.pll1 = {
		.pllm =		81,	/* 24 * (81 * 2) = 3888 */
		.prediv =	7,	/* 3888 / (7 + 1) = 486 */
		.postdiv =	0x00,
		.divs =		Mhz297_pll1_divs,
		.ndivs =	ARRAY_SIZE(Mhz297_pll1_divs)
	},
	.pll2 = {
		.pllm =		0x63,	/* 24 * (99 * 2) = 4752 */
		.prediv =	0x07,	/* 4752 / (7 + 1) = 594 */
		.postdiv =	0x00,
		.divs =		Mhz297_pll2_divs,
		.ndivs =	ARRAY_SIZE(Mhz297_pll2_divs)
	},
	/* The magic value selects arm from pll2 and other random stuff */
	.clkctrl = 0x243F04FC
};

/*
 * PLL configuration for board with 270Mhz cpu freq
 */

/* Input here is 432MHz */
static struct pll_postdivs Mhz270_pll1_divs[] = {
	{ PLL_PLLDIV2,  1},	/* 432/2 = 216 -> HDVICP */
	{ PLL_PLLDIV3,  1},	/* 432/2 = 216 -> MJCP,HDVICP */
	{ PLL_PLLDIV4,  3},	/* 432/4 = 108 -> EDMA and periph */
	{ PLL_PLLDIV5,  1},	/* 432/2 = 216 -> VPSS */
	{ PLL_PLLDIV6, 17},	/* 432/18 = 24 -> VENC */
	{ PLL_PLLDIV7,  0},	/* 432 -> DDR (post divided by 2 = 216 */
	{ PLL_PLLDIV8,  3},	/* 432/4 = 108 -> MMC0 */
	{ PLL_PLLDIV9,  1},	/* 432/2 = 216 -> CLKOUT */
};

/* Input here is 540 MHz */
static struct pll_postdivs Mhz270_pll2_divs[] = {
	{ PLL_PLLDIV2,  1},	/* 540/2 = 270 -> ARM */
	{ PLL_PLLDIV4, 28},	/* 540/29 = 18 -> VOICE */
	{ PLL_PLLDIV5,  7},	/* 540/8 = 67 -> VIDEO */
};

static struct pll_config Mhz270_pll_config = {
	.pll1 = {
		.pllm =		0x12,	/* 24 * (18 * 2) = 864 */
		.prediv =	0x1,	/* 864 / (1 + 1) = 432 */
		.postdiv =	0x00,
		.divs =		Mhz270_pll1_divs,
		.ndivs =	ARRAY_SIZE(Mhz270_pll1_divs)
	},
	.pll2 = {
		.pllm =		0x5a,	/* 24 * (90 * 2) = 4320 */
		.prediv =	0x07,	/* 4320 / (7 + 1) = 540 */
		.postdiv =	0x00,
		.divs =		Mhz270_pll2_divs,
		.ndivs =	ARRAY_SIZE(Mhz270_pll2_divs)
	},
	/* The magic value selects arm from pll2 and other random stuff */
	.clkctrl = 0x243F04FC
};


struct pll_config *board_pll_get_config(int * adcvals)
{
	int freq;

	freq = freqs[adcvals[4]%5];
	switch (freq) {
	case (432):
	case (297):
		return &Mhz297_pll_config;
		break;
	case (270):
		return &Mhz270_pll_config;
		break;
	case (216):
	default:
		return &Mhz270_pll_config; /* It should be 216Mhz */
	}
}

void board_dump_config(int *adcvals_norm)
{
	printk("Board:    %s\n", boards[adcvals_norm[2]+10*adcvals_norm[3]]);
	printk("Hw Vers:  %i\n", adcvals_norm[1]);
	printk("Boot dev: %s\n", bootings[adcvals_norm[4]/5]);
	printk("Cpu Freq: %iMhz\n", freqs[adcvals_norm[4]%5]);
}

int board_boot_cfg_get_config(int *adcvals, int *adcvals_n)
{
	int i;

	for (i = 0; i < 6; i++)
		adcvals_n[i] = normalize_adc(adcvals[i]);

	return 0; /* passing only first char */
}

