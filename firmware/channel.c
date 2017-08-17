////////////////////////////////////////////////////////////////
#include "quadvca.h"

CHAN_STATE g_chan[4];
CHAN_CFG g_chan_cfg[4];

unsigned int release_table[10] = {
	0x1000,
	0x0400,
	0x0200,
	0x00E0,
	0x00C0,
	0x00A0,
	0x0080,
	0x0060,
	0x0040,
	0x0020
};

////////////////////////////////////////////////////////////////
// TRIGGER ENVELOPE ON CHANNEL
void chan_trig(byte which) {
	CHAN_STATE *chan = &g_chan[which];
	switch(chan->env_state) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
			chan->env_state = ENV_ST_ATTACK;
			break;
		case ENV_ST_ATTACK:
		case ENV_ST_SUSTAIN:
			break;
	}
}

////////////////////////////////////////////////////////////////
// UNTRIGGER ENVELOPE ON CHANNEL
void chan_untrig(byte which) {
	CHAN_STATE *chan = &g_chan[which];
	switch(chan->env_state) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
		case ENV_ST_ATTACK:
			break;
		case ENV_ST_SUSTAIN:
			chan->env_state = ENV_ST_RELEASE;
			break;
	}
}
////////////////////////////////////////////////////////////////
// ONCE PER MILLISECOND PROCESSING
void chan_run(byte which) {

	// edge detection on CV inputs
	byte *cv_in = &adc_cv_state[which];
	if(*cv_in & ADC_CV_RISING_EDGE) {
		*cv_in &= ~ADC_CV_RISING_EDGE;
		chan_trig(which);
	}
	else if(*cv_in & ADC_CV_FALLING_EDGE) {
		*cv_in &= ~ADC_CV_FALLING_EDGE;
		chan_untrig(which);
	}	
	
	// periodic processing
	CHAN_STATE *chan = &g_chan[which];
	long l;
	switch(chan->env_state) {
		case ENV_ST_ATTACK:
			l = (long)chan->env_value + chan->env_attack;
			if(l >= 0xFFFF) {
				chan->env_value = 0xFFFF;
				chan->env_state = ENV_ST_RELEASE;
			}
			else {
				chan->env_value = (ENV_COUNTER)l;
			}
			break;
		case ENV_ST_RELEASE:
			l = (long)chan->env_value - chan->env_release;
			if(l <= 0) {
				chan->env_value = 0;
				chan->env_state = ENV_ST_ATTACK;
				//chan->env_state = ENV_ST_IDLE;
			}
			else {
				chan->env_value = (ENV_COUNTER)l;
			}
			break;
		case ENV_ST_IDLE:
		case ENV_ST_SUSTAIN:
			break;
	}
	
	vca_set(which, chan->env_value>>6);
}

////////////////////////////////////////////////////////////////
// INITIALISE CHANNEL
void chan_init(byte which) {

	CHAN_CFG *chan_cfg = &g_chan_cfg[which];
	chan_cfg->attack = 0;
	chan_cfg->sustain = 0;
	chan_cfg->release = 0;

	CHAN_STATE *chan = &g_chan[which];
	chan->env_mode = ENV_MODE_AR;
	chan->env_state = ENV_ST_IDLE;
	chan->env_value = 0;
	chan->env_attack = 0xFFFF;
	chan->env_release = 0x0080;
} 

byte clamp(byte d, byte max) {
	if(d > max) {
		return max;
	}
	else {
		return d;
	}
}



////////////////////////////////////////////////////////////////
// SET A PARAMETER
void chan_set(byte which, byte param, byte value) {
	CHAN_CFG *chan_cfg = &g_chan_cfg[which];
	CHAN_STATE *chan = &g_chan[which];
	switch(param) {
		case P_ATTACK:			
			chan_cfg->attack = clamp(value,10);
			break;
		case P_SUSTAIN:
			chan_cfg->sustain  = clamp(value,10);
			break;
		case P_RELEASE:
			chan_cfg->release  = clamp(value,10);
			chan->env_release = release_table[value];
			break;
	}	
};

////////////////////////////////////////////////////////////////
// GET A PARAMETER
byte chan_get(byte which, byte param) {
	CHAN_CFG *chan_cfg = &g_chan_cfg[which];
	switch(param) {
		case P_ATTACK:
			return chan_cfg->attack;
		case P_SUSTAIN:
			return chan_cfg->sustain;
		case P_RELEASE:
			return chan_cfg->release;
	}
	return 0;
};