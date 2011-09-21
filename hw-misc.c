#include <bubl/types.h>
#include <bubl/pll.h>
#include <bubl/hw.h>
#include <bubl/hw-misc.h>

/*
 * Misc device management, to clean up. Mainly from "device.c", by Sandeep
 * Paulraj, found in the dm3xx_sd_boot-6.1 package.
 * Partly reordered and cleaned up from crappy "corporate-style" code.
 */

#define TMPBUF		(unsigned int *)(0x17ff8)
#define TMPSTATUS	(unsigned int *)(0x17ff0)
#define FLAG_PORRST	0x00000001
#define FLAG_WDTRST	0x00000002
#define FLAG_FLGON	0x00000004
#define FLAG_FLGOFF	0x0000001

/* This initial part is copied from here and there, to make the rest happy */
static void DEVICE_pinmuxControl(u32 regOffset, u32 mask, u32 value)
{
	SYSTEM->PINMUX[regOffset] &= ~mask;
	SYSTEM->PINMUX[regOffset] |= (mask & value);
}

static void VPSS_SYNC_RESET(void)
{
	unsigned int PdNum = 0;

	SYSTEM->VPSS_CLKCTL |= 0x00000080;
	// VPSS_CLKMD 1:1

	//LPSC SyncReset DDR Clock Enable
	PSC->MDCTL[47] = ((PSC->MDCTL[47] & 0xffffffe0) | 0x00000001);

	PSC->PTCMD = (1<<PdNum);

	while(! (((PSC->PTSTAT >> PdNum) & 0x00000001) == 0))
		;

	while(!((PSC->MDSTAT[47] &  0x0000001F) == 0x1))
		;
}

#define GPINT_GPEN	(unsigned int *)(0x01C21C08)	// WDT special function
#define GPTDAT_GPDIR	(unsigned int *)(0x01C21C0c)	// WDT special function

static void POR_RESET(void)
{
	if (pll1_base[PLL_RSTYPE] & 3) {
		VPSS_SYNC_RESET();  // VPSS sync reset
		*TMPBUF = 0;
		*TMPSTATUS |= FLAG_PORRST;
		*GPINT_GPEN = 0x00020000;
		*GPTDAT_GPDIR = 0x00020002;
		while (1)
			;
	}
}

void WDT_RESET()
{
	volatile unsigned int s;

	if (*TMPBUF == 0x591b3ed7) {
		*TMPBUF = 0;
		*TMPSTATUS |= FLAG_PORRST;
		*TMPSTATUS |= FLAG_FLGOFF;

		for (s = 0; s < 0x100; s++)
			;
		VPSS_SYNC_RESET();
		*GPINT_GPEN = 0x00020000;	/* WDT */
		*GPTDAT_GPDIR = 0x00020002;	/* execute > */
		while (1)
			;
	}
}

void WDT_FLAG_ON()
{
	SYSTEM->VPSS_CLKCTL &= 0xffffff7f;      /* VPSS_CLKMD 1:2 */
	*TMPBUF = 0x591b3ed7;
	*TMPSTATUS |= FLAG_FLGON;
}

static void psc_turn_on(int id)
{
	/* wait for previous transitions to complete (GOSTAT bit) */
	while (PSC->PTSTAT & 1)
		;
	/* set next to on and start transition (GO bit) */
	PSC->MDCTL[id] |= 3;
	PSC->PTCMD = 1;

	/* wait for this transitions to complete (GOSTAT bit) */
	while (PSC->PTSTAT & 1)
		;
	/* wait for the device to be in state 3 (needed?) */
	while ((PSC->MDSTAT[id] & 0x1f) != 3)
		;
}

void DEVICE_PSCInit() /* power and sleep controller, chapter 7 of arm-subsys */
{
	unsigned char i=0;
	unsigned char lpsc_start;
	unsigned char lpsc_end,lpscgroup,lpscmin,lpscmax;
	unsigned int  PdNum = 0;

	lpscmin  =0;
	lpscmax  =2;

	for(lpscgroup=lpscmin ; lpscgroup <=lpscmax; lpscgroup++) {
		if(lpscgroup==0)
		{
			lpsc_start = 0; // Enabling LPSC 3 to 28 SCR first
			lpsc_end   = 28;
		}
		else if (lpscgroup == 1) { /* Skip locked LPSCs [29-37] */
			lpsc_start = 38;
			lpsc_end   = 47;
		} else {
			lpsc_start = 50;
			lpsc_end   = 51;
		}

		//NEXT=0x3, Enable LPSC's
		for(i=lpsc_start; i<=lpsc_end; i++) {
			PSC->MDCTL[i] |= 0x3;
		}

		//Program goctl to start transition sequence for LPSCs
		PSC->PTCMD = (1<<PdNum);

		//Wait for GOSTAT = NO TRANSITION from PSC for Powerdomain 0
		while(! (((PSC->PTSTAT >> PdNum) & 0x00000001) == 0))
			;

		//Wait for MODSTAT = ENABLE from LPSC's
		for(i=lpsc_start; i<=lpsc_end; i++) {
			while(!((PSC->MDSTAT[i] &  0x0000001F) == 0x3))
				;
		}
	}

}


/* The following two functions are called by bubl_main() */
int misc_setup0(void)
{
	// Mask all interrupts
	AINTC->INTCTL = 0x4;
	AINTC->EABASE = 0x0;
	AINTC->EINT0  = 0x0;
	AINTC->EINT1  = 0x0;

	// Clear all interrupts
	AINTC->FIQ0 = 0xFFFFFFFF;
	AINTC->FIQ1 = 0xFFFFFFFF;
	AINTC->IRQ0 = 0xFFFFFFFF;
	AINTC->IRQ1 = 0xFFFFFFFF;

	POR_RESET();
	WDT_RESET();

	// System PSC setup - enable all
	if (1) {
		DEVICE_PSCInit();
	} else {
		psc_turn_on(19); /* uart == 19 */
		psc_turn_on(13); /* sdram == 13 */
		psc_turn_on(15); /* mmc == 15 */
		psc_turn_on(26); /* gpio == 26 */
	}
	return 0;
}

void pinmux_setup(int bootmode)
{
	if (!bootmode) {
	/* eMMC */
		/*
		 * BASI Board pinmux
		 * (Default direction for GIOs is input)
		 * MMC reset released (43)
		 * Eth reset off (44),
		 * boot flash on (45),
		 */
		DEVICE_pinmuxControl(0,0xFFFFFFFF,0x00007F55);

		DEVICE_pinmuxControl(1,0xFFFFFFFF,0x00430000);
		DEVICE_pinmuxControl(2,0xFFFFFFFF,0x00001F80);

		/* EMAC, UART0 */
		DEVICE_pinmuxControl(3,0xFFFFFFFF,0x001A0000);

		DEVICE_pinmuxControl(4,0xFFFFFFFF,0x00000000);
	/* --- */
	} else {
	/* NAND */
		DEVICE_pinmuxControl(0, 0xFFFFFFFF, 0x00007F55);
		DEVICE_pinmuxControl(1, 0xFFFFFFFF, 0x00430000);
		DEVICE_pinmuxControl(2, 0xFFFFFFFF, 0x00001840);
		DEVICE_pinmuxControl(3, 0xFFFFFFFF, 0x001AFFFF);
		DEVICE_pinmuxControl(4, 0xFFFFFFFF, 0x00000000);
	/* --- */
	}
}

int misc_setup1(void)
{
	AEMIF->AWCCR = 0xff;
	AEMIF->A1CR = 0x00100084;
	AEMIF->NANDFCR |= 1;
	AEMIF->A2CR = 0x00a00505;
	return 0;
}

