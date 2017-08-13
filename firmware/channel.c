#include "quadvca.h"


void chan_run(byte which, VCA_CHANNEL *chan) {
	byte *cv_in = &adc_cv_state[which];
	if(*cv_in & ADC_CV_RISING_EDGE) {
		*cv_in &= ~ADC_CV_RISING_EDGE;
		env_trig(&chan->env);
	}
	else if(*cv_in & ADC_CV_FALLING_EDGE) {
		*cv_in &= ~ADC_CV_FALLING_EDGE;
		env_untrig(&chan->env);
	}	
	env_run(&chan->env);
	vca_set(which, ENV_VALUE(&chan->env));
}
void chan_init(VCA_CHANNEL *chan) {
	env_init(&chan->env);
}

void chan_set(VCA_CHANNEL *chan, byte param, byte value) {
	switch(param) {
		case P_ATTACK:
			chan->attack = value;
			break;
		case P_SUSTAIN:
			chan->sustain  = value;
			break;
		case P_RELEASE:
			chan->release  = value;
			break;
	}	
};

