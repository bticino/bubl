#ifndef __BUBL_PLL_H__
#define __BUBL_PLL_H__
#include <bubl/types.h>

enum pll_regs {
	PLL_PID,
	/* unused area */

	PLL_RSTYPE = (0xe4 / 4),
	/* more unused regs*/

	PLL_PLLCTL = (0x100 / 4),
	PLL_OCSEL,
	PLL_SECCTL,
	PLL_UNUSED_10c,

	PLL_PLLM,	/* 0x110 */
	PLL_PREDIV,
	PLL_PLLDIV1,
	PLL_PLLDIV2,

	PLL_PLLDIV3,	/* 0x120 */
	PLL_OSCDIV1,
	PLL_POSTDIV,
	PLL_BPDIV,

	PLL_UNUSED_130,	/* 0x130 */
	PLL_UNUSED_134,
	PLL_PLLCMD,
	PLL_PLLSTAT,

	PLL_ALNCTL,	/* 0x140 */
	PLL_DCHANGE,
	PLL_CKEN,
	PLL_CKSTAT,

	PLL_SYSTAT,	/* 0x150 */

	PLL_PLLDIV4 = (0x160 / 4),
	PLL_PLLDIV5,
	PLL_PLLDIV6,
	PLL_PLLDIV7,
	PLL_PLLDIV8,
	PLL_PLLDIV9
};

#define pll1_base ((volatile u32 *)0x01C40800)
#define pll2_base ((volatile u32 *)0x01C40C00)

/* The following structures describe the pll config */
struct pll_postdivs {
	u32 addr;
	u32 value;
};

struct pll_config_one {
	u32 prediv;
	u32 postdiv;
	u32 pllm;
	struct pll_postdivs *divs;
	int ndivs;
};

struct pll_config {
	struct pll_config_one pll1;
	struct pll_config_one pll2;
	u32 clkctrl;
};

extern int pll_bypass(void);
extern int pll_setup(struct pll_config *cfg);

#endif /* __BUBL_PLL_H__ */
