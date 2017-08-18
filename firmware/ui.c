#include <system.h>
#include "quadvca.h"

// the different UI modes
enum {
	UI_IDLE,
	UI_EDIT_CHAN_A,
	UI_EDIT_CHAN_B,
	UI_EDIT_CHAN_C,
	UI_EDIT_CHAN_D
};
#define NUM_LEDS 6

static byte led_2_disp[NUM_LEDS] = {LED_TRIG1, LED_TRIG2, LED_TRIG3, LED_TRIG4, LED_CLK, LED_ACT };

static byte digit_2_disp[10] = {CHAR_0, CHAR_1, CHAR_2, CHAR_3, CHAR_4, CHAR_5, CHAR_6, CHAR_7, CHAR_8, CHAR_9};
byte cur_param = 0;
static byte led_timeout[6] = {0};
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
			cur_param = P_FIRST;
			ui_mode = UI_EDIT_CHAN_A;
			return 1;
		case KEY_CHAN_A:
		case KEY_CHAN_B:
		case KEY_CHAN_C:
		case KEY_CHAN_D:
			chan_trig(key_2_chan(key));
			break;
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
			if(++cur_param > P_LAST) {
				cur_param = P_FIRST;
			}
			break;
		//////////////////////////////////////////
		// Decrement value
		case KEY_MINUS:
			value = chan_get(chan, cur_param);
			if(value) {
				chan_set(chan, cur_param, value - 1);
			}
			break;
		//////////////////////////////////////////
		// Increment value
		case KEY_PLUS:
			value = chan_get(chan, cur_param);
			chan_set(chan, cur_param, value + 1);
			break;
		//////////////////////////////////////////
		// Exit from channel editing
		case KEY_MODE:
			ui_mode = UI_IDLE;
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
				chan_trig(value);
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
		case P_HOLD:
			led_buf[0] = CHAR_H;
			led_buf[1] = CHAR_L|SEG_DP;
			break;
		case P_REPEAT:
			led_buf[0] = CHAR_R;
			led_buf[1] = CHAR_P|SEG_DP;
			break;
		case P_DENSITY:
			led_buf[0] = CHAR_D;
			led_buf[1] = CHAR_N|SEG_DP;
			break;
	}
	value = chan_get(chan, cur_param);
	led_buf[2] = digit_2_disp[value];
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


// to be called once every 1 ms 
void ui_tick() {

	byte d = 0;
	// deal with LEDs that have been switched on for a 
	// finite blink time
	for(byte i=0; i<NUM_LEDS; ++i) {
		if(led_timeout[i]) {
			d |= led_2_disp[i];
			--led_timeout[i];
		}
	}

	// now override LED states according to mode if needed
	switch(ui_mode) {
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
