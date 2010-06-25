#include <bubl/types.h>
#include <bubl/hw.h>
/*
 * PLL setup for DM365. Mainly from "device.c", by Sandeep Paulraj,
 * found in the dm3xx_sd_boot-6.1 package.
 * Partly reordered and cleaned up from crappy "corporate-style" code.
 */


int pll1_setup(void)
{
	unsigned int CLKSRC=0x0;

	/*Power up the PLL*/
	PLL1->PLLCTL &= 0xFFFFFFFD;

	PLL1->PLLCTL &= 0xFFFFFEFF;
	PLL1->PLLCTL |= CLKSRC<<8;

	/*Set PLLENSRC '0', PLL Enable(PLLEN)  controlled through MMR*/
	PLL1->PLLCTL &= 0xFFFFFFDF;

	/*Set PLLEN=0 => PLL BYPASS MODE*/
	PLL1->PLLCTL &= 0xFFFFFFFE;

	trivial_loop(150);

	// PLLRST=1(reset assert)
	PLL1->PLLCTL |= 0x00000008;

	trivial_loop(300);

	/*Bring PLL out of Reset*/
	PLL1->PLLCTL &= 0xFFFFFFF7;

	//Program the Multiper and Pre-Divider for PLL1
	PLL1->PLLM   =   0x51;   // VCO will 24*2M/N+1 = 486Mhz
	PLL1->PREDIV =   0x8000|0x7;

	// Assert TENABLE = 1, TENABLEDIV = 1, TINITZ = 1
	PLL1->SECCTL = 0x00470000;

	// Assert TENABLE = 1, TENABLEDIV = 1, TINITZ = 0
	PLL1->SECCTL = 0x00460000;

	// Assert TENABLE = 0, TENABLEDIV = 0, TINITZ = 0
	PLL1->SECCTL = 0x00400000;

	// Assert TENABLE = 0, TENABLEDIV = 0, TINITZ = 1
	PLL1->SECCTL = 0x00410000;

	//Program the PostDiv for PLL1
	PLL1->POSTDIV = 0x8000;

	// Post divider setting for PLL1
	PLL1->PLLDIV2 = 0x8001;
	PLL1->PLLDIV3 = 0x8001;   // POST DIV 486/2  -> MJCP/HDVICP
	PLL1->PLLDIV4 = 0x8003;   // POST DIV 486/4  -> EDMA/EDMA CFG
	PLL1->PLLDIV5 = 0x8001;   // POST DIV 486/2 -> VPSS
	PLL1->PLLDIV6 = 0x8011;   // 27Mhz POST DIV 486/18  -> VENC
        PLL1->PLLDIV7 = 0x8000;   // POST DIV 486/2 -> DDR
	PLL1->PLLDIV8 = 0x8003;   // POST DIV 486/4 -> MMC0/SD0
	PLL1->PLLDIV9 = 0x8001;   // POST DIV 486/2 -> CLKOUT

	trivial_loop(300);

	/*Set the GOSET bit */
	PLL1->PLLCMD = 0x00000001;  // Go

	trivial_loop(300);

	/*Wait for PLL to LOCK */
	while(! (((SYSTEM->PLL0_CONFIG) & 0x07000000) == 0x07000000))
		;

        /*Enable the PLL Bit of PLLCTL*/
	PLL1->PLLCTL |= 0x00000001;   // PLLEN=0

	return 0;
}

int pll2_setup(void)
{
	unsigned int CLKSRC=0x0;
	/*Power up the PLL*/
	PLL2->PLLCTL &= 0xFFFFFFFD;

	/*Select the Clock Mode as Onchip Oscilator or External Clock */
	/*VDB has input on MXI pin */

	PLL2->PLLCTL &= 0xFFFFFEFF;
	PLL2->PLLCTL |= CLKSRC<<8;

	/*Set PLLENSRC '0', PLL Enable(PLLEN)  is controlled through MMR*/
	PLL2->PLLCTL &= 0xFFFFFFDF;

	/*Set PLLEN=0 => PLL BYPASS MODE*/
	PLL2->PLLCTL &= 0xFFFFFFFE;

	trivial_loop(50);

	// PLLRST=1(reset assert)
	PLL2->PLLCTL |= 0x00000008;

	trivial_loop(300);

	/*Bring PLL out of Reset*/
	PLL2->PLLCTL &= 0xFFFFFFF7;

	//Program the Multiper and Pre-Divider for PLL2
	PLL2->PLLM   =   0x63;   // VCO will 24*2M/N+1 = 594Mhz
	PLL2->PREDIV =   0x8000|0x7;

	PLL2->POSTDIV = 0x8000;

	PLL2->SECCTL = 0x00470000;   // TENABLE = 1, TENABLEDIV = 1, TINITZ = 1
	PLL2->SECCTL = 0x00460000;   // TENABLE = 1, TENABLEDIV = 1, TINITZ = 0
	PLL2->SECCTL = 0x00400000;   // TENABLE = 0, TENABLEDIV = 0, TINITZ = 0
	PLL2->SECCTL = 0x00410000;   // TENABLE = 0, TENABLEDIV = 0, TINITZ = 1

	// Post divider setting for PLL2

	PLL2->PLLDIV2 = 0x8001;   // 594/2 =297 Mhz -> ARM
	PLL2->PLLDIV4 = 0x801C;   // POST DIV 594/29 = 20.48 -> VOICE
	PLL2->PLLDIV5 = 0x8007;   // POST DIV 594/8 = 74.25 ->VIDEO HD

	//GoCmd for PostDivider to take effect
	PLL2->PLLCMD = 0x00000001;
	trivial_loop(150);

	/*Wait for PLL to LOCK */
	while(! (((SYSTEM->PLL1_CONFIG) & 0x07000000) == 0x07000000))
		;

	trivial_loop(4100);

	//Enable the PLL2
	PLL2->PLLCTL |= 0x00000001;   // PLLEN=0

	//do this after PLL's have been set up
	SYSTEM->PERI_CLKCTRL = 0x243F04FC;

	return 0;

}

