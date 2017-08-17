


enum {
	P_ATTACK,
	P_SUSTAIN,
	P_RELEASE,
	
	P_FIRST = P_ATTACK,
	P_LAST = P_RELEASE
	
};

typedef unsigned char byte;
typedef unsigned int ENV_COUNTER;
enum {
	ENV_ST_IDLE,
	ENV_ST_ATTACK,
	ENV_ST_SUSTAIN,
	ENV_ST_RELEASE
};

enum {
	ENV_MODE_AR,
	ENV_MODE_ASR
};

typedef struct {
	byte env_mode;
	byte env_state;
	ENV_COUNTER env_value;	
	ENV_COUNTER env_attack; 
	ENV_COUNTER env_release; 	
} CHAN_STATE;

typedef struct {
	byte attack;
	byte sustain;
	byte release;
	byte dummy;
} CHAN_CFG;

extern CHAN_STATE g_chan[4];
extern CHAN_CFG g_chan_cfg[4];
extern unsigned int adc_cv_result[4];
extern byte adc_cv_state[4];
extern byte led_buf[4];


// CHANNELS
void chan_trig(byte which);
void chan_untrig(byte which);
void chan_run(byte which);
void chan_init(byte which);
void chan_set(byte which, byte param, byte value);
byte chan_get(byte which, byte param);

void ui_notify(byte key, byte modifiers);
void ui_run();

void adc_run();
void vca_set(byte which, unsigned int level);




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

enum {
	SEG_A  = 0x20,
	SEG_B  = 0x80,
	SEG_C  = 0x02,
	SEG_D  = 0x10,
	SEG_E  = 0x08,
	SEG_F  = 0x40,
	SEG_G  = 0x04,
	SEG_DP = 0x01
};


enum {
	CHAR_0	= SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,
	CHAR_1	= SEG_B|SEG_C,
	CHAR_2	= SEG_A|SEG_B|SEG_D|SEG_E|SEG_G,
	CHAR_3	= SEG_A|SEG_B|SEG_C|SEG_D|SEG_G,
	CHAR_4	= SEG_B|SEG_C|SEG_F|SEG_G,
	CHAR_5	= SEG_A|SEG_C|SEG_D|SEG_F|SEG_G,
	CHAR_6	= SEG_A|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,
	CHAR_7	= SEG_A|SEG_B|SEG_C,
	CHAR_8	= SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,
	CHAR_9	= SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,

	/*
	 AAA
    F   B
     GGG     
    E   C
     DDD
*/

	CHAR_A	= SEG_A|SEG_B|SEG_C|SEG_E|SEG_F|SEG_G,	
	CHAR_B	= SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,
	CHAR_C	= SEG_A|SEG_D|SEG_E|SEG_F,
	CHAR_D	= SEG_B|SEG_C|SEG_D|SEG_E|SEG_G,
	CHAR_E	= SEG_A|SEG_D|SEG_E|SEG_F|SEG_G,
	CHAR_F	= SEG_A|SEG_E|SEG_F|SEG_G,	
	CHAR_G	= SEG_A|SEG_C|SEG_D|SEG_E|SEG_F,	
	CHAR_H	= SEG_B|SEG_C|SEG_E|SEG_F|SEG_G,	
	CHAR_I	= SEG_B|SEG_C,
	CHAR_J	= SEG_B|SEG_C|SEG_D,	
	CHAR_K	= SEG_A|SEG_C|SEG_E|SEG_F|SEG_G,	
	CHAR_L	= SEG_D|SEG_E|SEG_F,	
	CHAR_M	= SEG_A|SEG_C|SEG_E|SEG_G,
	CHAR_N	= SEG_A|SEG_B|SEG_C|SEG_E|SEG_F,
	CHAR_O	= SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,	
	CHAR_P	= SEG_A|SEG_B|SEG_E|SEG_F|SEG_G,	
	CHAR_Q	= SEG_A|SEG_B|SEG_C|SEG_F|SEG_G,	
	CHAR_R	= SEG_A|SEG_E|SEG_F,	
	CHAR_S	= SEG_A|SEG_C|SEG_D|SEG_F|SEG_G,	
	CHAR_T	= SEG_D|SEG_E|SEG_F|SEG_G,	
	CHAR_U	= SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,	
	CHAR_V	= SEG_C|SEG_D|SEG_E,
	CHAR_W	= SEG_A|SEG_C|SEG_D|SEG_E,
	CHAR_X	= SEG_B|SEG_D|SEG_F|SEG_G,
	CHAR_Y	= SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,
	CHAR_Z	= SEG_A|SEG_B|SEG_D|SEG_E|SEG_G
	
};

enum {
	VCA1,
	VCA2,
	VCA3,
	VCA4
};

enum {
	VCA_CLOSED = 0,
	VCA_OPEN = 1023
};

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



