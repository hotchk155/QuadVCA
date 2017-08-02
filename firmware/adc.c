#include <system.h>
#include "quadvca.h"

unsigned int adc_cv_result[4] = {0}; // 10-bit (0-1023) ADC result for each gate
byte adc_cv_state[4] = {0};

static byte adc_chan = 0xFF; // current ADC channel
static byte adc_delay = 0; // counter used to force delay for cap to charge
static byte adc_lower_threshold = 200;
static byte adc_upper_threshold = 1000;


///////////////////////////////////////////////////////
// ADC STATE MACHINE
void adc_run() 
{	
	enum {
		CHAN_GATE1	= 0b00101001,	// 01010 AN10
		CHAN_GATE2  = 0b00011001,	// 00110 AN6
		CHAN_GATE3  = 0b00010101,	// 00101 AN5
		CHAN_GATE4  = 0b00010001	// 00100 AN4
	};
	
	if(adc_delay > 0) {	// waiting for ADC acquisition delay
		if(--adc_delay == 0) {
			// start conversion
			adcon0.1 = 1;
		}
	}
	else if(!adcon0.1) { // conversion complete
		if(adc_chan == 0xFF) { // no result to store
			adc_chan = 0;
		}
		else { 
			byte *state_bits = &adc_cv_state[adc_chan];
			unsigned int result = adresh << 8 | adresl;			
			
			// store conversion result and indicate that a 
			// reading is available
			adc_cv_result[adc_chan] = result;
			(*state_bits) |= ADC_CV_RESULT;
			
			// detect rising and falling edges, applying a 
			// hysteresis
			if((*state_bits) & ADC_CV_HIGH) {
				if(result <= adc_lower_threshold) {
					(*state_bits) &= ~ADC_CV_HIGH;
					(*state_bits) |= ADC_CV_FALLING_EDGE;
				}
			}
			else {
				if(result >= adc_upper_threshold) {
					(*state_bits) |= (ADC_CV_HIGH | ADC_CV_RISING_EDGE);
				}
			}			

			// next channel
			if(++adc_chan > 3) {
				adc_chan = 0;
			}
		}
		
		// connect ADC to appropriate channel
		switch(adc_chan) {
			case 0: adcon0 = CHAN_GATE1; break;
			case 1: adcon0 = CHAN_GATE2; break;
			case 2: adcon0 = CHAN_GATE3; break;
			case 3: adcon0 = CHAN_GATE4; break;
		}
		
		// force a delay before we start the conversion
		// to give time for ADC sample and hold cap to charge
		adc_delay = 10;
	}
}

