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
	SLOPE_MAXENUM
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
	CH_OPT_NONE,	
	CH_OPT_SLAVE,		//1
	CH_OPT_LINK,		//2
	CH_OPT_CHOKE,		//3
	CH_OPT_INV,			//4
	CH_OPT_LINK_INV,	//5
	CH_OPT_MAXENUM
};

#define TRIG_MASK_MAX  10

byte trig_masks[TRIG_MASK_MAX] = {
		0b00000001,
		0b11011011,
		0b00110111,
		0b10110110,
		0b01000100,
		0b00110100,
		0b10101101,
		0b00100110,
		0b01010000,
		0b01011101
};


word slopes[SLOPE_MAXENUM] = {
	INC_INSTANT, INC_FASTEST, INC_FAST, INC_MEDIUM, INC_SLOW, INC_SLOWEST
};

typedef struct {
	
	// Config params 
	byte option;	
	byte trig_mask;	
	byte trig_mask_cycle;	
	byte trig_mask_index;	
	
	byte attack_slope;
	byte sustain;
	byte release_slope;
		
	word attack_inc; 
	word release_inc; 	

	// Running status
	byte is_trig;		// trigger flag
	byte env_phase;		// envelope phase
	word env_level;		// current value
	word env_max;		// maximum envelope level 

} CHANNEL;



static CHANNEL channels[CHAN_MAX];
static byte cur_chan = 0;

byte chan_mixer_mode = 0;





////////////////////////////////////////////////////////////////
// SET VCA OUTPUT LEVEL FOR A CHANNEL
// level is 0-65535 and will be scaled for 10 bit VCA out
// scale parameter is 0 for no scaling, 1 for env scaling
void chan_vca(byte which, word level) {
	if(channels[which].option == CH_OPT_INV ||
		channels[which].option == CH_OPT_LINK_INV) {
		level = 65535 - level;
	}
	vca(which, level>>6);
}

////////////////////////////////////////////////////////////////
// TRIGGER ENVELOPE ON CHANNEL.. we expect a later untrigger
void chan_trig(byte which) {
	CHANNEL *this_chan = &channels[which];
	this_chan->is_trig = 1;
	//ui_blink_led(which);	
	chan_ping(which);	
}

////////////////////////////////////////////////////////////////
// TRIGGER ENVELOPE ON CHANNEL
void chan_ping(byte which) {

	CHANNEL *this_chan = &channels[which];	
	
	// If this channel is directly linked to the level of another 
	// channel then we do not trigger its evelopes
	if (this_chan->option == CH_OPT_LINK ||
		this_chan->option == CH_OPT_LINK_INV) {
		return;
	}
	
	// Check if a trigger mask is active on the channel
	if(!chan_mixer_mode) {
		byte d = (trig_masks[this_chan->trig_mask] & (1<<this_chan->trig_mask_index));
		if(++this_chan->trig_mask_index >= this_chan->trig_mask_cycle) {
			this_chan->trig_mask_index = 0;
		}
		if(!d) {
			return;
		}
	}
	
	// loop through the targeted channel and subsequent channels that have
	// triggers that are slaved to it
	for(byte chan = which; chan < 4; ++chan) {
	
		this_chan = &channels[chan];
		CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;

		// when we reach the first following channel that is not a trigger
		// slave, we can stop the search
		if(chan > which && this_chan->option != CH_OPT_SLAVE) {
			break;
		}

		ui_blink_led(chan);		

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
					chan_vca(chan, this_chan->env_level);
				}/*
				else {
					this_chan->env_level = 0;
				}*/
				//chan_vca(chan, this_chan->env_level);
				break;
		}
	}
	
	// now look for following channels that have a "choke"
	for(byte slave = which + 1; slave < 4 && channels[slave].option == CH_OPT_CHOKE; ++slave) {
		chan_reset(slave);		
	}
	
}

////////////////////////////////////////////////////////////////
// UNTRIGGER ENVELOPE ON CHANNEL
void chan_untrig(byte which) {

	CHANNEL *this_chan = &channels[which];
	
	// ignore if a linked channel
	if (this_chan->option == CH_OPT_LINK ||
		this_chan->option == CH_OPT_LINK_INV) {
		return;
	}
	
	// loop through the targeted channel and subsequent channels that have
	// triggers that are slaved to it
	for(byte chan = which; chan < 4; ++chan) {
	
		this_chan = &channels[chan];
		CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;
	
		// when we reach the first following channel that is not a trigger
		// slave, we can stop the search
		if(chan > which && this_chan->option != CH_OPT_SLAVE) {
			break;
		}
	
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
// RESET CYCLE COUNTS
void chan_reset_cycle(byte which) {
	CHANNEL *this_chan = &channels[which];
	this_chan->trig_mask_index = 0;
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

	// check whether this channel is linked to a lower numbered channel and 
	// should take its level from that channel
	for(int i=cur_chan;
		i>0 && this_chan->option == CH_OPT_LINK || this_chan->option == CH_OPT_LINK_INV;
		--i) {
		this_chan = &channels[i-1];
	}
	
	chan_vca(cur_chan, this_chan->env_level);		
	
	if(++cur_chan >= CHAN_MAX) {
		cur_chan =  0;
	}

	
}



////////////////////////////////////////////////////////////////
// INITIALISE CHANNEL
void chan_init() {
	for(byte i=0; i<CHAN_MAX; ++i) {
		CHANNEL *this_chan = &channels[i];
		memset(this_chan, 0, sizeof(CHANNEL));
		this_chan->release_slope = SLOPE_MEDIUM;
		this_chan->trig_mask = 0;
		this_chan->trig_mask_cycle = 1;	

		chan_reset(i);
	}
	
//channels[1].option = CH_OPT_LINK_INV;
} 


////////////////////////////////////////////////////////////////
// STORE A PARAMETER
void chan_set(byte which, byte param, int value) {
	CHANNEL *this_chan = &channels[which];
	CHANNEL *env_chan = chan_mixer_mode? &channels[0] : this_chan;
	switch(param) {
		case P_ATTACK:			
			env_chan->attack_slope = clamp(value, 0, SLOPE_MAXENUM-1);
			break;
		case P_SUSTAIN:
			env_chan->sustain = clamp(value, 0, 1);
			break;
		case P_RELEASE:
			env_chan->release_slope = clamp(value, 0, SLOPE_MAXENUM-1);
			break;
		case P_OPTION:
			this_chan->option = clamp(value, 0, CH_OPT_MAXENUM-1);
			break;
		case P_CYCLE:
			this_chan->trig_mask_cycle = clamp(value, 1, 8);
			break;
		case P_MASK:
			this_chan->trig_mask = clamp(value, 0, TRIG_MASK_MAX-1);
			break;
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
		case P_OPTION:
			return this_chan->option;
		case P_CYCLE:
			return this_chan->trig_mask_cycle;
		case P_MASK:
			return this_chan->trig_mask;
	}
	return 0;
};