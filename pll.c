#include <bubl/types.h>
#include <bubl/pll.h>
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

static int pll1_setup(void)
{
	unsigned int CLKSRC=0x0;

	/*Power up the PLL*/
	pll1_base[PLL_PLLCTL] &= 0xFFFFFFFD;

	pll1_base[PLL_PLLCTL] &= 0xFFFFFEFF;
	pll1_base[PLL_PLLCTL] |= CLKSRC<<8;

	/*Set PLLENSRC '0', PLL Enable(PLLEN)  controlled through MMR*/
	pll1_base[PLL_PLLCTL] &= 0xFFFFFFDF;

	/*Set PLLEN=0 => PLL BYPASS MODE*/
	pll1_base[PLL_PLLCTL] &= 0xFFFFFFFE;

	trivial_loop(150);

	// PLLRST=1(reset assert)
	pll1_base[PLL_PLLCTL] |= 0x00000008;

	trivial_loop(300);

	/*Bring PLL out of Reset*/
	pll1_base[PLL_PLLCTL] &= 0xFFFFFFF7;

	//Program the Multiper and Pre-Divider for PLL1
	pll1_base[PLL_PLLM]   =   0x51;   // VCO will 24*2M/N+1 = 486Mhz
	pll1_base[PLL_PREDIV] =   0x8000|0x7;

	// Assert TENABLE = 1, TENABLEDIV = 1, TINITZ = 1
	pll1_base[PLL_SECCTL] = 0x00470000;

	// Assert TENABLE = 1, TENABLEDIV = 1, TINITZ = 0
	pll1_base[PLL_SECCTL] = 0x00460000;

	// Assert TENABLE = 0, TENABLEDIV = 0, TINITZ = 0
	pll1_base[PLL_SECCTL] = 0x00400000;

	// Assert TENABLE = 0, TENABLEDIV = 0, TINITZ = 1
	pll1_base[PLL_SECCTL] = 0x00410000;

	//Program the PostDiv for PLL1
	pll1_base[PLL_POSTDIV] = 0x8000;

	// Post divider setting for PLL1
	pll1_base[PLL_PLLDIV2] = 0x8001;
	pll1_base[PLL_PLLDIV3] = 0x8001;   // POST DIV 486/2  -> MJCP/HDVICP
	pll1_base[PLL_PLLDIV4] = 0x8003;   // POST DIV 486/4  -> EDMA/EDMA CFG
	pll1_base[PLL_PLLDIV5] = 0x8001;   // POST DIV 486/2 -> VPSS
	pll1_base[PLL_PLLDIV6] = 0x8011;   // 27Mhz POST DIV 486/18  -> VENC
        pll1_base[PLL_PLLDIV7] = 0x8000;   // POST DIV 486/2 -> DDR
	pll1_base[PLL_PLLDIV8] = 0x8003;   // POST DIV 486/4 -> MMC0/SD0
	pll1_base[PLL_PLLDIV9] = 0x8001;   // POST DIV 486/2 -> CLKOUT

	trivial_loop(300);

	/*Set the GOSET bit */
	pll1_base[PLL_PLLCMD] = 0x00000001;  // Go

	trivial_loop(300);

	/*Wait for PLL to LOCK */
	while(! (((SYSTEM->PLL0_CONFIG) & 0x07000000) == 0x07000000))
		;

        /*Enable the PLL Bit of PLLCTL*/
	pll1_base[PLL_PLLCTL] |= 0x00000001;   // PLLEN=0

	return 0;
}

static int pll2_setup(void)
{
	unsigned int CLKSRC=0x0;
	/*Power up the PLL*/
	pll2_base[PLL_PLLCTL] &= 0xFFFFFFFD;

	/*Select the Clock Mode as Onchip Oscilator or External Clock */
	/*VDB has input on MXI pin */

	pll2_base[PLL_PLLCTL] &= 0xFFFFFEFF;
	pll2_base[PLL_PLLCTL] |= CLKSRC<<8;

	/*Set PLLENSRC '0', PLL Enable(PLLEN)  is controlled through MMR*/
	pll2_base[PLL_PLLCTL] &= 0xFFFFFFDF;

	/*Set PLLEN=0 => PLL BYPASS MODE*/
	pll2_base[PLL_PLLCTL] &= 0xFFFFFFFE;

	trivial_loop(50);

	// PLLRST=1(reset assert)
	pll2_base[PLL_PLLCTL] |= 0x00000008;

	trivial_loop(300);

	/*Bring PLL out of Reset*/
	pll2_base[PLL_PLLCTL] &= 0xFFFFFFF7;

	//Program the Multiper and Pre-Divider for PLL2
	pll2_base[PLL_PLLM]   =   0x63;   // VCO will 24*2M/N+1 = 594Mhz
	pll2_base[PLL_PREDIV] =   0x8000|0x7;

	pll2_base[PLL_POSTDIV] = 0x8000;

	pll2_base[PLL_SECCTL] = 0x00470000;   // TENABLE = 1, TENABLEDIV = 1, TINITZ = 1
	pll2_base[PLL_SECCTL] = 0x00460000;   // TENABLE = 1, TENABLEDIV = 1, TINITZ = 0
	pll2_base[PLL_SECCTL] = 0x00400000;   // TENABLE = 0, TENABLEDIV = 0, TINITZ = 0
	pll2_base[PLL_SECCTL] = 0x00410000;   // TENABLE = 0, TENABLEDIV = 0, TINITZ = 1

	// Post divider setting for PLL2

	pll2_base[PLL_PLLDIV2] = 0x8001;   // 594/2 =297 Mhz -> ARM
	pll2_base[PLL_PLLDIV4] = 0x801C;   // POST DIV 594/29 = 20.48 -> VOICE
	pll2_base[PLL_PLLDIV5] = 0x8007;   // POST DIV 594/8 = 74.25 ->VIDEO HD

	//GoCmd for PostDivider to take effect
	pll2_base[PLL_PLLCMD] = 0x00000001;
	trivial_loop(150);

	/*Wait for PLL to LOCK */
	while(! (((SYSTEM->PLL1_CONFIG) & 0x07000000) == 0x07000000))
		;

	trivial_loop(4100);

	//Enable the PLL2
	pll2_base[PLL_PLLCTL] |= 0x00000001;   // PLLEN=0

	//do this after PLL's have been set up
	SYSTEM->PERI_CLKCTRL = 0x243F04FC;

	return 0;

}

int pll_setup(struct pll_config *cfg)
{
    pll1_setup();
    pll2_setup();
    return 0;
}
