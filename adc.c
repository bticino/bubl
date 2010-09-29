#include <bubl/types.h>
#include <bubl/adc.h>

#define adc_base ((volatile u32 *)0x01c23c00) /* Timer 1 (0 used by uboot) */


void adc_setup(void)
{
	adc_base[ADC_SETDIV] = 0x100; /* not too slow */
}


u32 adc_read(int channel)
{
	adc_base[ADC_CHSEL] = (1 << channel);
	adc_base[ADC_ADCTL] = 1; /* start */
	while (adc_base[ADC_ADCTL] & (1 << 7))
		/* busy */;
	return adc_base[ADC_DAT(channel)];


}
