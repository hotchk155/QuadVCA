#include <system.h>
#include "quadvca.h"

static byte led_4_chan[4] = {LED_TRIG1, LED_TRIG2, LED_TRIG3, LED_TRIG4 };

void ui_run() {
	
}

void ui_sel_chan(byte which) {
	led_buf[3] &= ~(LED_TRIG1|LED_TRIG2|LED_TRIG3|LED_TRIG4);
	led_buf[3] |= led_4_chan[which];
}

void ui_keypress(byte key, byte modifiers) {
	switch(key) {
		case KEY_SELECT:
		case KEY_MINUS:
		case KEY_PLUS:
		case KEY_MODE:
			break;
		case KEY_CHAN_A: ui_sel_chan(0); break;			
		case KEY_CHAN_B: ui_sel_chan(1); break;			
		case KEY_CHAN_C: ui_sel_chan(2); break;			
		case KEY_CHAN_D: ui_sel_chan(3); break;			
	}
}

