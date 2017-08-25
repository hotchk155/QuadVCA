#include <system.h>
#include "quadvca.h"

// the different UI modes
enum {
	UI_IDLE,
	UI_SELECT_MODE,
	UI_EDIT_CHAN_A,
	UI_EDIT_CHAN_B,
	UI_EDIT_CHAN_C,
	UI_EDIT_CHAN_D
};
#define NUM_LEDS 6
#define LED_BLINK_TIME 2

enum {
	LEDBIT_CLK		= 0x01,
	LEDBIT_ACT		= 0x10,
	LEDBIT_TRIG1	= 0x80,
	LEDBIT_TRIG2	= 0x08,
	LEDBIT_TRIG3	= 0x20,
	LEDBIT_TRIG4	= 0x40
};
static byte led_2_disp[NUM_LEDS] = {LEDBIT_TRIG1, LEDBIT_TRIG2, LEDBIT_TRIG3, LEDBIT_TRIG4, LEDBIT_CLK, LEDBIT_ACT };


static byte digit_2_disp[10] = {CHAR_0, CHAR_1, CHAR_2, CHAR_3, CHAR_4, CHAR_5, CHAR_6, CHAR_7, CHAR_8, CHAR_9};
byte cur_param = 0;
byte global_param = 0;
byte ui_mode = UI_IDLE;
byte ui_repaint = 1;


static byte key_2_chan(byte which) {
	switch(which) {
		case KEY_CHAN_B: return 1;			
		case KEY_CHAN_C: return 2;
		case KEY_CHAN_D: return 3;
		case KEY_CHAN_A: default: return 0;
	}
}


// Set up the 7-segment display and channel LEDs 

//////////////////////////////////////////////////////////////////////
// RUN "IDLE" MODE
// Called for a keypress, or once per millisecond (key = 0)
static byte ui_mode_idle(byte key, byte modifiers) {
	switch(key) {
		case 0:
			led_buf[0] = SEG_DP;
			led_buf[1] = SEG_DP;
			led_buf[2] = SEG_DP;
			break;
		case KEY_SELECT:
			cur_param = 0;
			ui_mode = UI_EDIT_CHAN_A;
			return 1;
		case KEY_CHAN_A:
		case KEY_CHAN_B:
		case KEY_CHAN_C:
		case KEY_CHAN_D:
			chan_trig(key_2_chan(key));
			break;
		case KEY_MODE:
			ui_mode = UI_SELECT_MODE;
			return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
// KEYPRESS HANDLER (CHANNEL EDIT MODE)
static byte ui_mode_edit_chan(byte key, byte modifiers) {
	byte chan = ui_mode - UI_EDIT_CHAN_A;
	byte value;
	switch(key) {
		//////////////////////////////////////////
		// Select between channel params
		case KEY_SELECT:
			if(++cur_param >= P_MAXENUM) {
				cur_param = 0;
			}
			break;
		//////////////////////////////////////////
		// Decrement value
		case KEY_MINUS:
			chan_set(chan, cur_param, chan_get(chan, cur_param) - 1);
			break;
		//////////////////////////////////////////
		// Increment value
		case KEY_PLUS:
			chan_set(chan, cur_param, chan_get(chan, cur_param) + 1);
			break;
		//////////////////////////////////////////
		// Exit from channel editing
		case KEY_MODE:
			ui_mode = UI_SELECT_MODE;
			return 1;
			
		//////////////////////////////////////////
		// Press a channel button
		case KEY_CHAN_A:
		case KEY_CHAN_B:
		case KEY_CHAN_C:
		case KEY_CHAN_D:
			value = key_2_chan(key);
			if(ui_mode == (UI_EDIT_CHAN_A + value)) {
				// if channel is already selected, the gate is triggered
				chan_ping(value);
				break;
			}
			else {
				// the channel becomes selected
				ui_mode = UI_EDIT_CHAN_A + value;
				return 1;
			}
	}
	
	// Update the channel edit display
	switch(cur_param) {
		case P_ATTACK:
			led_buf[0] = CHAR_A;
			led_buf[1] = CHAR_T|SEG_DP;
			break;
		case P_SUSTAIN:
			led_buf[0] = CHAR_S;
			led_buf[1] = CHAR_T|SEG_DP;
			break;
		case P_RELEASE:
			led_buf[0] = CHAR_R;
			led_buf[1] = CHAR_L|SEG_DP;
			break;
		case P_CYCLE:
			led_buf[0] = CHAR_C;
			led_buf[1] = CHAR_Y|SEG_DP;
			break;
		case P_MASK:
			led_buf[0] = CHAR_P;
			led_buf[1] = CHAR_T|SEG_DP;
			break;
		case P_OPTION:
			led_buf[0] = CHAR_O;
			led_buf[1] = CHAR_P|SEG_DP;
			break;
	}
	value = chan_get(chan, cur_param);
	led_buf[2] = digit_2_disp[value];
	return 0;
}

/*
//////////////////////////////////////////////////////////////////////
// 
static byte ui_mode_edit_globals(byte key, byte modifiers) {
	byte value;
	switch(key) {
		//////////////////////////////////////////
		// Select between channel params
		case KEY_SELECT:
			if(++global_param >= GLOBAL_MAXENUM) {
				global_param = 0;
			}
			break;
		//////////////////////////////////////////
		// Decrement value
		case KEY_MINUS:
			global_set(global_param, global_get(global_param) - 1);
			break;
		//////////////////////////////////////////
		// Increment value
		case KEY_PLUS:
			global_set(global_param, global_get(global_param) + 1);
			break;
		//////////////////////////////////////////
		// Exit from channel editing
		case KEY_MODE:
			if(modifiers & KEY_SELECT) {
				ui_mode = UI_SELECT_MODE;
				return 1;
			}
			else {
				ui_mode = UI_IDLE;
				return 1;
			}
		//////////////////////////////////////////
		// Press a channel button
		case KEY_CHAN_A:
		case KEY_CHAN_B:
		case KEY_CHAN_C:
		case KEY_CHAN_D:
			// the channel becomes selected
			value = key_2_chan(key);			
			ui_mode = UI_EDIT_CHAN_A + value;
			return 1;
	}
	
	// Update the channel edit display
	switch(global_param) {
		case GLOBAL_SHARE_ENV:
			led_buf[0] = CHAR_S;
			led_buf[1] = CHAR_H|SEG_DP;
			break;
	}
	value = global_get(global_param);
	led_buf[2] = digit_2_disp[value];
	return 0;
}
*/

//////////////////////////////////////////////////////////////////////
// CHANGE UI MODE
static byte ui_mode_select(byte key, byte modifiers) {
	switch(key) {
		case KEY_MODE:
		case KEY_SELECT:
			ui_mode = UI_IDLE;
			return 1;
		case KEY_MINUS:
			set_run_mode(get_run_mode() - 1);
			break;
		case KEY_PLUS:
			set_run_mode(get_run_mode() + 1);
			break;
		case KEY_CHAN_A:
		case KEY_CHAN_B:
		case KEY_CHAN_C:
		case KEY_CHAN_D:
			// the channel becomes selected
			ui_mode = UI_EDIT_CHAN_A + key_2_chan(key);
			return 1;
	}

	switch(get_run_mode()) {
		case RUN_MODE_TRIG:
			led_buf[0] = CHAR_T;
			led_buf[1] = CHAR_R;
			led_buf[2] = CHAR_G;
			break;
		case RUN_MODE_TRIG_MIX:
			led_buf[0] = CHAR_T;
			led_buf[1] = CHAR_R;
			led_buf[2] = CHAR_2;
			break;
		case RUN_MODE_TOGGLE:
			led_buf[0] = CHAR_T;
			led_buf[1] = CHAR_O;
			led_buf[2] = CHAR_G;
			break;
		case RUN_MODE_TOGGLE_MIX:
			led_buf[0] = CHAR_T;
			led_buf[1] = CHAR_G;
			led_buf[2] = CHAR_2;
			break;
		case RUN_MODE_TRIGCV:
			led_buf[0] = CHAR_T;
			led_buf[1] = CHAR_C;
			led_buf[2] = CHAR_V;
			break;
		case RUN_MODE_TRIGCV_MIX:
			led_buf[0] = CHAR_T;
			led_buf[1] = CHAR_C;
			led_buf[2] = CHAR_2;
			break;
		case RUN_MODE_SOLOTRIG:
			led_buf[0] = CHAR_P;
			led_buf[1] = CHAR_R;
			led_buf[2] = CHAR_T;
			break;
		case RUN_MODE_SOLOTRIG_MIX:
			led_buf[0] = CHAR_P;
			led_buf[1] = CHAR_R;
			led_buf[2] = CHAR_2;
			break;
		case RUN_MODE_FADERSCV:
			led_buf[0] = CHAR_F;
			led_buf[1] = CHAR_A;
			led_buf[2] = CHAR_D;
			break;
		case RUN_MODE_XFADECV:
			led_buf[0] = CHAR_C;
			led_buf[1] = CHAR_R;
			led_buf[2] = CHAR_F;
			break;
		case RUN_MODE_STEP:
			led_buf[0] = CHAR_S;
			led_buf[1] = CHAR_T;
			led_buf[2] = CHAR_P;
			break;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
// KEYPRESS HANDLER
void ui_notify(byte key, byte modifiers) {

		
	switch(ui_mode) {
		case UI_IDLE:
			if(ui_mode_idle(key, modifiers)) {
				ui_repaint = 1;
			}
			break;
		case UI_SELECT_MODE:
			if(ui_mode_select(key, modifiers)) {
				ui_repaint = 1;
			}
			break;
		case UI_EDIT_CHAN_A:
		case UI_EDIT_CHAN_B:
		case UI_EDIT_CHAN_C:
		case UI_EDIT_CHAN_D:
			if(ui_mode_edit_chan(key, modifiers)) {
				ui_repaint = 1;
			}
			break;
	}
}

void ui_blink_led(byte which) {
	switch(which) {
		case LED_CHAN1: led_timeout[7] = LED_BLINK_TIME; break;
		case LED_CHAN2: led_timeout[3] = LED_BLINK_TIME; break;
		case LED_CHAN3: led_timeout[5] = LED_BLINK_TIME; break;
		case LED_CHAN4: led_timeout[6] = LED_BLINK_TIME; break;
		case LED_CLOCK: led_timeout[0] = LED_BLINK_TIME; break;
		case LED_ACT: led_timeout[4] = LED_BLINK_TIME; break;
	}
	/*
	byte x[6] = {7, 3, 5, 6, 0, 4};
	if(x<6) {
		led_timeout[x[which]] = LED_BLINK_TIME;	
	}*/
}

// to be called once every 1 ms 
void ui_tick() {

	byte d = 0;
	
	// LED states according to mode 
	switch(ui_mode) {
		case UI_SELECT_MODE:
//			d |= (LED_TRIG1|LED_TRIG2|LED_TRIG3|LED_TRIG4);
			break;
		case UI_EDIT_CHAN_A:
		case UI_EDIT_CHAN_B:
		case UI_EDIT_CHAN_C:
		case UI_EDIT_CHAN_D:	
			// when editing any channel
			d |= led_2_disp[ui_mode - UI_EDIT_CHAN_A];
			break;
	}
	led_buf[3] = d;
	
	// force the display to be updated
	if(ui_repaint) {
		ui_notify(0,0);
		ui_repaint = 0;
	}
}
