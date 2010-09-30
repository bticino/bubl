#include <bubl/pll.h>
#include <bubl/tools.h>
#include "board.h"

/*
 * PLL configuration for BMX board
 */

/* Input here is 486MHz */
static struct pll_postdivs bmx_pll1_divs[] = {
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
static struct pll_postdivs bmx_pll2_divs[] = {
	{ PLL_PLLDIV2,  1},	/* 594/2 = 297 -> ARM */
	{ PLL_PLLDIV4, 28},	/* 594/29 = 20.4827 -> VOICE */
	{ PLL_PLLDIV5,  7},	/* 594/8 = 74.25 -> VIDEO */
};

static struct pll_config bmx_pll_config = {
	.pll1 = {
		.pllm =		81,	/* 24 * (81 * 2) = 3888 */
		.prediv =	7,	/* 3888 / (7 + 1) = 486 */
		.postdiv =	0x00,
		.divs =		bmx_pll1_divs,
		.ndivs =	ARRAY_SIZE(bmx_pll1_divs)
	},
	.pll2 = {
		.pllm =		0x63,	/* 24 * (99 * 2) = 4752 */
		.prediv =	0x07,	/* 4752 / (7 + 1) = 594 */
		.postdiv =	0x00,
		.divs =		bmx_pll2_divs,
		.ndivs =	ARRAY_SIZE(bmx_pll2_divs)
	},
	/* The magic value selects arm from pll2 and other random stuff */
	.clkctrl = 0x243F04FC
};

struct pll_config *board_get_config(int *adcvals)
{
	/* Currently I only have the BMX board so I ignore adcvals */
	return &bmx_pll_config;
}
