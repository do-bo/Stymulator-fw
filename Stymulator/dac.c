#include <compiler.h>
#include <dac.h>

struct dac_config conf;
	uint8_t           i = 0;

	board_init();
	sysclk_init();

	// Initialize the dac configuration.
	dac_read_configuration(&SPEAKER_DAC, &conf);

	/* Create configuration:
	* - 1V from bandgap as reference, left adjusted channel value
	* - one active DAC channel, no internal output
	* - conversions triggered by event channel 0
	* - 1 us conversion intervals
	*/
	dac_set_conversion_parameters(&conf, DAC_REF_BANDGAP, DAC_ADJ_LEFT);
	dac_set_active_channel(&conf, SPEAKER_DAC_CHANNEL, 0);
	dac_set_conversion_trigger(&conf, SPEAKER_DAC_CHANNEL, 0);
#if XMEGA_DAC_VERSION_1
	dac_set_conversion_interval(&conf, 1);
#endif
	dac_write_configuration(&SPEAKER_DAC, &conf);
	dac_enable(&SPEAKER_DAC);
	
#if XMEGA_E
	// Configure timer/counter to generate events at sample rate.
	sysclk_enable_module(SYSCLK_PORT_C, SYSCLK_TC4);
	TCC4.PER = (sysclk_get_per_hz() / RATE_OF_CONVERSION) - 1;

	// Configure event channel 0 to generate events upon T/C overflow.
	sysclk_enable_module(SYSCLK_PORT_GEN, SYSCLK_EVSYS);
	EVSYS.CH0MUX = EVSYS_CHMUX_TCC4_OVF_gc;

	// Start the timer/counter.
	TCC4.CTRLA = TC45_CLKSEL_DIV1_gc;
#else
	// Configure timer/counter to generate events at sample rate.
	sysclk_enable_module(SYSCLK_PORT_C, SYSCLK_TC0);
	TCC0.PER = (sysclk_get_per_hz() / RATE_OF_CONVERSION) - 1;

	// Configure event channel 0 to generate events upon T/C overflow.
	sysclk_enable_module(SYSCLK_PORT_GEN, SYSCLK_EVSYS);
	EVSYS.CH0MUX = EVSYS_CHMUX_TCC0_OVF_gc;

	// Start the timer/counter.
	TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
#endif

	/* Write samples to the DAC channel every time it is ready for new
	 * data, i.e., when it is done converting. Conversions are triggered by
	 * the timer/counter.
	 */
	do {
		dac_wait_for_channel_ready(&SPEAKER_DAC, SPEAKER_DAC_CHANNEL);

		dac_set_channel_value(&SPEAKER_DAC, SPEAKER_DAC_CHANNEL, sine[i]);

		i++;
		i %= NR_OF_SAMPLES;
	} while (1);