
#include <bubl/hw.h>
#include <bubl/hw-misc.h>

static void DEVICE_LPSCTransition(int module, int domain, int state)
{
	/* Wait for any outstanding transition to complete */
	while ((PSC->PTSTAT) & (0x00000001 << domain))
		;

	/* If we are already in that state, just return */
	if (((PSC->MDSTAT[module]) & 0x1F) == state)
		return;

	/* Perform transition */
	PSC->MDCTL[module] = ((PSC->MDCTL[module]) & (0xFFFFFFE0)) | (state);
	PSC->PTCMD |= (0x00000001 << domain);

	/* Wait for transition to complete */
	while ((PSC->PTSTAT) & (0x00000001 << domain))
		;

	/* Wait and verify the state */
	while (((PSC->MDSTAT[module]) & 0x1F) != state)
		;
}

int ddr_setup(void)
{
	DEVICE_LPSCTransition(LPSC_DDR2, 0, PSC_ENABLE);

	SYSTEM->VTPIOCR = (SYSTEM->VTPIOCR) & 0xFFFF9F3F;

	/* Set bit CLRZ (bit 13) */
	SYSTEM->VTPIOCR = (SYSTEM->VTPIOCR) | 0x00002000;

	/* Check VTP READY Status */
	while (!(SYSTEM->VTPIOCR & 0x8000))
		;

	/* Set bit VTP_IOPWRDWN bit 14 for DDR input buffers)
	SYSTEM->VTPIOCR = SYSTEM->VTPIOCR | 0x00004000; */

	/* Set bit LOCK(bit7) and PWRSAVE (bit8) */
	SYSTEM->VTPIOCR = SYSTEM->VTPIOCR | 0x00000080;

	/* Powerdown VTP as it is locked (bit 6)
	   Set bit VTP_IOPWRDWN bit 14 for DDR input buffers) */
	SYSTEM->VTPIOCR = SYSTEM->VTPIOCR | 0x00004040;

	/* Wait for calibration to complete */
	nop(150);

	/* Set the DDR2 to synreset, then enable it again */
	DEVICE_LPSCTransition(LPSC_DDR2, 0, PSC_SYNCRESET);
	DEVICE_LPSCTransition(LPSC_DDR2, 0, PSC_ENABLE);

	DDR->DDRPHYCR = 0x000000C5;

	DDR->SDBCR = 0x08D34832;	/* SDRAM Bank Config Register */
	DDR->SDBCR = 0x0853C832;

	DDR->SDTIMR = 0x3C934B51;	/* SDRAM Timing Control Register1 */
	DDR->SDTIMR2 = 0x4221C72;	/* SDRAM Timing Control Register2 */

	DDR->PBBPR = 0x000000FE;

	DDR->SDBCR = 0x08534832;	/* SDRAM Bank Config Register */

	DDR->SDRCR = 0x00000768;	/* SDRAM Refresh Control Register */

	DEVICE_LPSCTransition(LPSC_DDR2, 0, PSC_SYNCRESET);
	nop(10);
	DEVICE_LPSCTransition(LPSC_DDR2, 0, PSC_ENABLE);

	return 0;
}
