#include <linux/kernel.h>

#include <mach/adc.h>

static uint32_t core_numerators = 0, core_denominators;
static uint16_t adc_dcdc;

static uint32_t vbat_numerators = 0, vbat_denominators;

extern int __is_valid_adc_cal(void);
extern int __adc2vbat(int adc_res);

bool htc_adc_is_calibrated(void)
{
	return __is_valid_adc_cal();
}

static int htc_bat_adc_to_vol(uint16_t adcvalue)
{
	return __adc2vbat(adcvalue);
}

int htc_adc_to_vol(int channel, uint16_t adc_value)
{
	u32 chan_numerators, chan_denominators;
	int voltage;

	sci_adc_get_vol_ratio(channel, true, &chan_numerators, &chan_denominators);

	if (!htc_adc_is_calibrated())
	{
		// no calibration data
		if (core_numerators == 0) {
			sci_adc_get_vol_ratio(ADC_CHANNEL_DCDCCORE, true, &core_numerators, &core_denominators);
			adc_dcdc = sci_adc_get_value(ADC_CHANNEL_DCDCCORE, true);
		}

		// DCDCCORE default 1100mV
		voltage = (chan_numerators == 0 ? 0 : adc_value * 1100 * (int)(core_numerators * chan_denominators) /(int) (core_denominators * chan_numerators) / adc_dcdc);
	}
	else
	{
		if (vbat_numerators == 0) {
			sci_adc_get_vol_ratio(ADC_CHANNEL_VBAT, true, &vbat_numerators, &vbat_denominators);
		}

		voltage = htc_bat_adc_to_vol(adc_value);
		voltage = (chan_numerators == 0 ? 0 : voltage *(int) (vbat_numerators * chan_denominators) / (int)(vbat_denominators * chan_numerators));
	}

	return voltage;
}
