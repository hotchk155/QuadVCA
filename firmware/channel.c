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

unsigned int slopes[SLOPE_MAX] = {
	INC_INSTANT, INC_FASTEST, INC_FAST, INC_MEDIUM, INC_SLOW, INC_SLOWEST
};

typedef struct {
	
	// Config params 
	//byte hold;
	//byte repeat;
	byte density;	
	byte attack_slope;
	byte sustain;
	byte release_slope;
		
	word attack_inc; 
	word release_inc; 	
//	word sustain_ms;	// time in milliseconds

	// Running status
	byte is_trig;		// trigger flag
	byte env_phase;		// envelope phase
	word env_level;		// current value
	word env_max;		// maximum envelope level 
//	word timeout;		

	byte count;
} CHANNEL;



static CHANNEL channels[CHAN_MAX];
static byte cur_chan = 0;

byte chan_mixer_mode = 0;





/*static void apply_config(CHANNEL *chan) {

	CHANNEL *env = g_shared_env? &channels[0] : chan;
	
	env->attack_slope = clamp(chan->attack_slope, 0, SLOPE_MAX-1);
	env->release_slope = clamp(chan->release_slope, 0, SLOPE_MAX-1);
	env->sustain = clamp(chan->sustain, 0, 1);
	env->attack_inc = slopes[chan->attack_slope];
	env->release_inc = slopes[chan->release_slope];
	
	chan->density = clamp(chan->density, 1, 8);
		
}*/

void chan_vca(byte which, unsigned int level) {

	if(level) {	
		level >>= 7; // into 0-511 range
		level += 512;
	}
	chan_vca_direct(which, level);
}

////////////////////////////////////////////////////////////////
// Set VCA to a level 0-65535 and map into the useful range of 
// the actual VCA controlling signal
void chan_vca_direct(byte which, unsigned int level) {

	
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
// TRIGGER ENVELOPE ON CHANNEL.. we expect a later untrigger
void chan_trig(byte which) {
	CHANNEL *this_chan = &channels[which];
	this_chan->is_trig = 1;
	chan_ping(which);
}

////////////////////////////////////////////////////////////////
// TRIGGER ENVELOPE ON CHANNEL
void chan_ping(byte which) {

	CHANNEL *this_chan = &channels[which];
	CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;
	ui_blink_led(which);	
	if(!chan_mixer_mode && this_chan->density > 1) {
		if(this_chan->count++ < this_chan->density)
			return;
		this_chan->count = 0;
	}
		
	switch(this_chan->env_phase) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
		case ENV_ST_ATTACK:
		case ENV_ST_SUSTAIN:
			this_chan->env_phase = ENV_ST_ATTACK;
			this_chan->attack_inc = slopes[env_chan->attack_slope];			
			this_chan->env_max = 0xFFFF;
			if(this_chan->attack_inc == 0xFFFF) {
				this_chan->env_level = this_chan->env_max;
			}
			else {
				this_chan->env_level = 0;
			}
			chan_vca(which, this_chan->env_level);
			break;
	}
}

////////////////////////////////////////////////////////////////
// UNTRIGGER ENVELOPE ON CHANNEL
void chan_untrig(byte which) {

	CHANNEL *this_chan = &channels[which];
	CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;
	
	switch(this_chan->env_phase) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
		case ENV_ST_ATTACK:
		case ENV_ST_SUSTAIN:
			this_chan->release_inc = slopes[env_chan->release_slope];		
			this_chan->env_phase = ENV_ST_RELEASE;
			break;
	}
	this_chan->is_trig = 0;
}

////////////////////////////////////////////////////////////////
// TOGGLE STATE OF A CHANNEL
void chan_toggle(byte which) {
	CHANNEL *this_chan = &channels[which];
	if(this_chan->is_trig) {
		chan_untrig(which);
	}
	else {
		chan_trig(which);
	}
}

////////////////////////////////////////////////////////////////
// RESET CHANNEL STATE
void chan_reset(byte which) {
	CHANNEL *this_chan = &channels[which];
	this_chan->env_phase = ENV_ST_IDLE;
	this_chan->env_level = 0;
	this_chan->is_trig = 0;
	chan_vca(which, 0);
}

////////////////////////////////////////////////////////////////
// Once per millisecond processing... channels processed on a 
// round-robin basis so each channel is processed once per 4ms
void chan_tick() {

	CHANNEL *this_chan = &channels[cur_chan];
	CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;
		
	long l;
	switch(this_chan->env_phase) {
	
		/////////////////////////////////////////////////////
		// ATTACK
		case ENV_ST_ATTACK:
			l = (long)this_chan->env_level + this_chan->attack_inc;
			if(l >= this_chan->env_max) {
				// attack phase is over...
				this_chan->env_level = this_chan->env_max;
				
				// is a sustain phase defined
				if(env_chan->sustain) {
					this_chan->env_phase = ENV_ST_SUSTAIN;
				}
				else {
					this_chan->release_inc = slopes[env_chan->release_slope];		
					this_chan->env_phase = ENV_ST_RELEASE;
				}
			}
			else {
				// store the new 16-bit envelope level
				this_chan->env_level = (word)l;
			}
			break;

		/////////////////////////////////////////////////////
		// SUSTAIN
		case ENV_ST_SUSTAIN:
			break;

		/////////////////////////////////////////////////////
		// RELEASE
		case ENV_ST_RELEASE:
			l = (long)this_chan->env_level - this_chan->release_inc;
			if(l <= 0) {
			
				// end of release phase
				this_chan->env_level = 0;
				this_chan->env_phase = ENV_ST_IDLE;
			}
			else {
				// store new counter value
				this_chan->env_level = (word)l;
			}
			break;

		/////////////////////////////////////////////////////
		default:
		case ENV_ST_IDLE:
			break;
	}
	
	chan_vca(cur_chan, this_chan->env_level);
	
	// prepare to move to next channel
	if(++cur_chan >= CHAN_MAX) {
		cur_chan =  0;
	}
}



////////////////////////////////////////////////////////////////
// INITIALISE CHANNEL
void chan_init() {
	for(byte i=0; i<CHAN_MAX; ++i) {
		CHANNEL *this_chan = &channels[cur_chan];
		memset(this_chan, 0, sizeof(CHANNEL));
		this_chan->release_slope = SLOPE_MEDIUM;
		chan_reset(i);
	}
} 


////////////////////////////////////////////////////////////////
// STORE A PARAMETER
void chan_set(byte which, byte param, int value) {
	CHANNEL *this_chan = &channels[which];
	CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;
	switch(param) {
		case P_ATTACK:			
			env_chan->attack_slope = clamp(value, 0, SLOPE_MAX-1);
			break;
		case P_SUSTAIN:
			env_chan->sustain = clamp(value, 0, 1);
			break;
		case P_RELEASE:
			env_chan->release_slope = clamp(value, 0, SLOPE_MAX-1);
			break;
		case P_DENSITY:
			this_chan->density = clamp(value, 0, 8);
	}	
}


////////////////////////////////////////////////////////////////
// GET A PARAMETER
int chan_get(byte which, byte param) {
	CHANNEL *this_chan = &channels[which];
	CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;
	switch(param) {
		case P_ATTACK:
			return env_chan->attack_slope;
		case P_SUSTAIN:
			return env_chan->sustain;
		case P_RELEASE:
			return env_chan->release_slope;
		case P_DENSITY:
			return this_chan->density;
	}
	return 0;
};