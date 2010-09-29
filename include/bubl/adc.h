/*
 * Register definitions for the adc peripheral
 */
#include <bubl/types.h>

enum adc_regs {
	ADC_ADCTL,
	ADC_CMPTGT,
	ADC_CMPLDAT,
	ADC_CMPUDAT,

	ADC_SETDIV,
	ADC_CHSEL,
	ADC_AD0DAT,
	ADC_AD1DAT,

	ADC_AD2DAT,
	ADC_AD3DAT,
	ADC_AD4DAT,
	ADC_AD5DAT,

	ADC_EMUCTRL
};

#define ADC_DAT(x) (ADC_AD0DAT + x)

extern void adc_setup(void);
extern u32 adc_read(int channel);
