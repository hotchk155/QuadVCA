////////////////////////////////////////////////////////////////
#include "quadvca.h"

enum {
	ENV_ST_IDLE,
	ENV_ST_ATTACK,
	ENV_ST_SUSTAIN,
	ENV_ST_RELEASE
};

enum {
	SLOPE_INSTANT,
	SLOPE_FASTEST,
	SLOPE_FAST,
	SLOPE_MEDIUM,
	SLOPE_SLOW,
	SLOPE_SLOWEST,
	SLOPE_MAX
};

enum {
	INC_INSTANT 	= 65535,
	INC_FASTEST 	= 1600,
	INC_FAST 		= 800,
	INC_MEDIUM 		= 400,
	INC_SLOW 		= 200,
	INC_SLOWEST 	= 100
};

enum {
	SUSTAIN_NONE,
	SUSTAIN_SHORTEST,
	SUSTAIN_SHORT,
	SUSTAIN_MEDIUM,
	SUSTAIN_LONG,
	SUSTAIN_LONGEST,
	SUSTAIN_UNLIMITED,
	SUSTAIN_MAX
};

enum {
	TIMEOUT_NONE = 0,
	TIMEOUT_SHORTEST = 100,
	TIMEOUT_SHORT = 250,
	TIMEOUT_MEDIUM = 500,
	TIMEOUT_LONG = 1000,
	TIMEOUT_LONGEST = 2000,
	TIMEOUT_UNLIMITED = 65535
};

unsigned int slopes[SLOPE_MAX] = {
	INC_INSTANT, INC_FASTEST, INC_FAST, INC_MEDIUM, INC_SLOW, INC_SLOWEST
};
unsigned int timeouts[SUSTAIN_MAX] = {
	TIMEOUT_NONE, TIMEOUT_SHORTEST,	TIMEOUT_SHORT, TIMEOUT_MEDIUM, TIMEOUT_LONG, TIMEOUT_LONGEST, TIMEOUT_UNLIMITED
};

typedef struct {
	
	// Config params 
	byte hold;
	byte repeat;
	byte density;
	
	byte attack_slope;
	byte sustain_time;
	byte release_slope;
		
	word attack_inc; 
	word release_inc; 	
	word sustain_ms;	// time in milliseconds

	// Running status
	byte is_trig;		// trigger flag
	byte env_phase;		// envelope phase
	word env_level;		// current value
	word env_max;		// maximum envelope level 
	word timeout;		

	byte count;
} CHANNEL;



static CHANNEL channels[CHAN_MAX];
static byte cur_chan = 0;





static byte clamp(byte d, byte min, byte max) {
	if(d < min) {
		return min;
	}
	if(d > max) {
		return max;
	}
	else {
		return d;
	}
}

static void apply_config(CHANNEL *chan) {

	chan->attack_slope = clamp(chan->attack_slope, 0, SLOPE_MAX-1);
	chan->release_slope = clamp(chan->release_slope, 0, SLOPE_MAX-1);
	chan->sustain_time = clamp(chan->sustain_time, 0, SUSTAIN_MAX-1);
	chan->hold = clamp(chan->hold, 0, 1);
	chan->repeat = clamp(chan->repeat, 0, 1);
	chan->density = clamp(chan->density, 1, 8);
		
	chan->attack_inc = slopes[chan->attack_slope];
	chan->release_inc = slopes[chan->release_slope];
	chan->sustain_ms = timeouts[chan->sustain_time];
}

////////////////////////////////////////////////////////////////
// Set VCA to a level 0-65535 and map into the useful range of 
// the actual VCA controlling signal
static void chan_2_vca(byte which, unsigned int level) {

	if(level) {	
		level >>= 7; // into 0-511 range
		level += 512;
	}
	
	// works in inverse sense
	level = 1023-level;
	
	switch(which) {
		case 0:
			ccpr2l = level>>2;
			ccp2con.5 = level.1;
			ccp2con.4 = level.0;	
			break;
		case 1:
			ccpr4l = level>>2;
			ccp4con.5 = level.1;
			ccp4con.4 = level.0;	
			break;
		case 2:
			ccpr3l = level>>2;
			ccp3con.5 = level.1;
			ccp3con.4 = level.0;	
			break;
		case 3:
			ccpr1l = level>>2;
			ccp1con.5 = level.1;
			ccp1con.4 = level.0;	
			break;
	}
}


////////////////////////////////////////////////////////////////
// TRIGGER ENVELOPE ON CHANNEL
void chan_trig(byte which) {

	CHANNEL *chan = &channels[which];
	if(chan->density > 1) {
		if(chan->count++ < chan->density)
			return;
		chan->count = 0;
	}
	
	
	switch(chan->env_phase) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
		case ENV_ST_ATTACK:
		case ENV_ST_SUSTAIN:
			chan->env_phase = ENV_ST_ATTACK;
			chan->env_max = 0xFFFF;
			if(chan->attack_inc == 0xFFFF) {
				chan->env_level = chan->env_max;
			}
			else {
				chan->env_level = 0;
			}
			chan_2_vca(which, chan->env_level);
			break;
	}
}

////////////////////////////////////////////////////////////////
// UNTRIGGER ENVELOPE ON CHANNEL
void chan_untrig(byte which) {
	CHANNEL *chan = &channels[which];
	switch(chan->env_phase) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
			break;
		case ENV_ST_ATTACK:
		case ENV_ST_SUSTAIN:
			if(!chan->hold) {
				// if the hold flag is set then the attack phase is always allowed
				// to complete. Otherwise we can end the attack phase early
				chan->env_phase = ENV_ST_RELEASE;
			}
			break;
	}
}

////////////////////////////////////////////////////////////////
// RESET CHANNEL STATE
void chan_reset(byte which) {
	channels[which].env_phase = ENV_ST_IDLE;
	channels[which].env_level = 0;
}

////////////////////////////////////////////////////////////////
// Once per millisecond processing... channels processed on a 
// round-robin basis so each channel is processed once per 4ms
void chan_tick() {
		
	// envelope state machine
	CHANNEL *chan = &channels[cur_chan];
	long l;
	switch(chan->env_phase) {
	
		/////////////////////////////////////////////////////
		// ATTACK
		case ENV_ST_ATTACK:
			l = (long)chan->env_level + chan->attack_inc;
			if(l >= chan->env_max) {
				// attack phase is over...
				chan->env_level = chan->env_max;
				chan->env_phase = ENV_ST_RELEASE;
				
				// is a sustain phase defined
				if(chan->sustain_ms) {
					if(chan->hold && chan->sustain_ms == SUSTAIN_UNLIMITED) {
						// this is an invalid combination, so fall into release
					}
					else {					
						// start the sustain phase
						chan->timeout = chan->sustain_ms;							
						chan->env_phase = ENV_ST_SUSTAIN;
					}
				}
			}
			else {
				// store the new 16-bit envelope level
				chan->env_level = (word)l;
			}
			break;

		/////////////////////////////////////////////////////
		// SUSTAIN
		case ENV_ST_SUSTAIN:
			// is a finite sustain timeout defined?
			if(chan->sustain_ms != SUSTAIN_UNLIMITED) {
			
				// take account of fact channels are processed 1 per ms
				// when decrementing the counter
				if(chan->timeout < CHAN_MAX) {
					chan->env_phase = ENV_ST_RELEASE;
					chan->timeout = 0;
				}
				else {
					chan->timeout -= CHAN_MAX;
				}
			}
			break;

		/////////////////////////////////////////////////////
		// RELEASE
		case ENV_ST_RELEASE:
			l = (long)chan->env_level - chan->release_inc;
			if(l <= 0) {
			
				// end of release phase
				chan->env_level = 0;
				
				// if the repeat flag is set and the trigger is still
				// present then we'll start the envelope again
				if(chan->repeat && chan->is_trig) {
					chan->env_phase = ENV_ST_ATTACK;
				}
				else {
					chan->env_phase = ENV_ST_IDLE;
				}
			}
			else {
				// store new counter value
				chan->env_level = (word)l;
			}
			break;

		/////////////////////////////////////////////////////
		default:
		case ENV_ST_IDLE:
			break;
	}
	
	chan_2_vca(cur_chan, chan->env_level);
	
	// prepare to move to next channel
	if(++cur_chan >= CHAN_MAX) {
		cur_chan =  0;
	}
}


////////////////////////////////////////////////////////////////
void chan_monitor() {
	// edge detection on CV inputs
	for(byte i=0; i<CHAN_MAX; ++i) {
		byte *cv_in = &adc_cv_state[i];
		if(*cv_in & ADC_CV_RISING_EDGE) {
			*cv_in &= ~ADC_CV_RISING_EDGE;
			// handle a trigger
			chan_trig(i);
			channels[i].is_trig = 1;
		}
		else if(*cv_in & ADC_CV_FALLING_EDGE) {
			*cv_in &= ~ADC_CV_FALLING_EDGE;
			// handle an untrigger
			chan_untrig(i);
			channels[i].is_trig = 0;
		}	
	}
}

////////////////////////////////////////////////////////////////
// INITIALISE CHANNEL
void chan_init() {
	for(byte i=0; i<CHAN_MAX; ++i) {
		CHANNEL *chan = &channels[i];
		memset(chan, 0, sizeof(CHANNEL));
		chan->release_slope = SLOPE_MEDIUM;
		apply_config(chan);
		chan_reset(i);
	}
} 



////////////////////////////////////////////////////////////////
// STORE A PARAMETER
void chan_set(byte which, byte param, byte value) {
	CHANNEL *chan = &channels[which];
	switch(param) {
		case P_ATTACK:			
			chan->attack_slope = value;
			break;
		case P_SUSTAIN:
			chan->sustain_time = value;
			break;
		case P_RELEASE:
			chan->release_slope = value;
			break;
		case P_HOLD:
			chan->hold = value;
			break;
		case P_REPEAT:
			chan->repeat = value;
			break;
		case P_DENSITY:
			chan->density = value;
	}	
	apply_config(chan);
	chan_reset(which);
}

////////////////////////////////////////////////////////////////
// GET A PARAMETER
byte chan_get(byte which, byte param) {
	CHANNEL *chan = &channels[which];
	switch(param) {
		case P_ATTACK:
			return chan->attack_slope;
		case P_SUSTAIN:
			return chan->sustain_time;
		case P_RELEASE:
			return chan->release_slope;
		case P_HOLD:
			return chan->hold;
		case P_REPEAT:
			return chan->repeat;
		case P_DENSITY:
			return chan->density;
	}
	return 0;
};