
#include <system.h>
#include <memory.h>
#include "chars.h"


enum {
	P_RELEASE,
	P_ATTACK,
	P_SUSTAIN,
	//P_HOLD,
	//P_REPEAT,
	P_DENSITY,	
	P_MAXENUM
	
};

enum {
	GLOBAL_SHARE_ENV,
	GLOBAL_MAXENUM
};



#define CHAN_MAX 4
typedef unsigned char byte;
typedef unsigned int word;


extern unsigned int adc_cv_result[4];
extern byte adc_cv_state[4];
extern byte led_buf[4];

extern byte chan_mixer_mode = 0;


byte clamp(byte d, byte min, byte max);
void global_set(byte param, int value);
int global_get(byte param);

// CHANNELS
void chan_ping(byte which);
void chan_trig(byte which);
void chan_untrig(byte which);
void chan_tick();
void chan_init();
void chan_set(byte which, byte param, int value);
int chan_get(byte which, byte param);
void chan_vca(byte which, unsigned int level);
void chan_vca_direct(byte which, unsigned int level);
void chan_toggle(byte which);

void ui_notify(byte key, byte modifiers);
void ui_tick();

void adc_run();




#define P_SRC1		latb.7		// active low enable for the first 7-segment display 
#define P_SRC2		latc.7		// active low enable for the second 7-segment display 
#define P_SRC3		latc.4		// active low enable for the third 7-segment display
#define P_SRC4		latb.6		// active low enable for MCU driven panel LED indicators 

#define P_SR_RCK	lata.5		// STORE clock
#define P_SR_SCK	lata.4		// SHIFT clock
#define P_SR_DAT 	P_SRC4		// Line is shared with one display source (active low)




#define P_PWM1		latc.3
#define P_PWM2		latc.6
#define P_PWM3		lata.2
#define P_PWM4		latc.5

#define TRIS_PWM1		trisc.3
#define TRIS_PWM2		trisc.6
#define TRIS_PWM3		trisa.2
#define TRIS_PWM4		trisc.5

#define P_GATE1			portb.4	// AN10
#define P_GATE2			portc.2 // AN6
#define P_GATE3			portc.1 // AN5
#define P_GATE4			portc.0 // AN4

#define P_SWITCH		porta.3

                        //76543210
#define DEF_ANSELA		0b00000000
#define DEF_ANSELB		0b00010000
#define DEF_ANSELC		0b00000111


					//76543210
#define TRIS_A		0b11001111
#define TRIS_B		0b00111111
#define TRIS_C		0b01101111


#define P_KB_READ		porta.1



enum {
	LED_CLK		= 0x01,
	LED_ACT		= 0x10,
	LED_TRIG1	= 0x80,
	LED_TRIG2	= 0x08,
	LED_TRIG3	= 0x20,
	LED_TRIG4	= 0x40
};


/*
enum {
	VCA1,
	VCA2,
	VCA3,
	VCA4
};

enum {
	VCA_CLOSED = 400, // value we consider as closed VCA
	VCA_OPEN = 1023
};
*/
enum {
	ADC_CV_RESULT 				= 0x01,	// set whenever there is a new result for input in adc_cv_result
	ADC_CV_HIGH					= 0x02, 
	ADC_CV_RISING_EDGE			= 0x04, 	
	ADC_CV_FALLING_EDGE			= 0x08
};


/*
Buttons: bits that show up in key_state
	A B C D
   E  F  G  H
*/
enum {
	K_A = 0x20,
	K_B = 0x04,
	K_C = 0x02,
	K_D = 0x10,
	K_E = 0x08,
	K_F = 0x80,
	K_G = 0x40,
	K_H = 0x01,
	
	KEY_SELECT	= K_A,
	KEY_MINUS	= K_B,
	KEY_PLUS	= K_C,
	KEY_MODE	= K_D,
	KEY_CHAN_A	= K_E,
	KEY_CHAN_B	= K_F,
	KEY_CHAN_C	= K_G,
	KEY_CHAN_D	= K_H
};



