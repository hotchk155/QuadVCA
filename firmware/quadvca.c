//

//
// INCLUDE FILES
//
#include "quadvca.h"

// CONFIG OPTIONS 
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_ON & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 32000000



/*
		
	
PIC PWM RESOURCES


RA2	/ CCP3
RC3 / CCP2
RC5 / CCP1
RC6 / CCP4


		
*/


//
// GLOBAL DATA
//
volatile byte led_buf[4] = {0};
volatile byte led_timeout[8] = {0};
volatile byte dx_buf[4] = {0};



//
// FILE SCOPE DATA
//
#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz
									// using this init scalar means that rollover
									// interrupt fires once per ms
static volatile byte tick_flag = 0; // flag set when millsecond interrupt fires


// definitions for the display update state machine
enum {
	DX_PREP_REFRESH,
	DX_REFRESH, 			
	DX_PREP_KEYSCAN,			
	DX_KEYSCAN
};
#define TIMER_INIT_DISPLAY_HOLD 	200	// display hold time (255 - timer1 ticks)
#define TIMER_INIT_KEY_SETTLE		254 // key input settle time (255 - timer1 ticks)
static volatile byte dx_state;
static volatile byte dx_disp;
static volatile byte dx_mask;
static volatile byte dx_key_state;
static volatile byte key_state = 0;

static byte trig_chan = 0;
static byte run_mode = RUN_MODE_TRIG;

#define RX_BUFFER_MASK 0x1F
volatile byte rx_buffer[32];
volatile byte rx_head = 0; 
volatile byte rx_tail = 0; 

// State flags used while receiving MIDI data
byte midi_status = 0;					// current MIDI message status (running status)
byte midi_num_params = 0;				// number of parameters needed by current MIDI message
byte midi_params[2];					// parameter values of current MIDI message
char midi_param = 0;					// number of params currently received

////////////////////////////////////////////////////////////
// INTERRUPT HANDLER 
void interrupt( void )
{

	byte i;
	////////////////////////////////////////////////////////////
	// timer 0 rollover ISR. Maintains the count of 
	// "system ticks" that we use for key debounce etc
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		tick_flag = 1;
		intcon.2 = 0;
	}

	////////////////////////////////////////////////////////////
	// Timer 1 interrupt
	// This interrupt drives the state machine that updates the 
	// LED displays
	if(pir1.0) 
	{
		pir1.0 = 0;
		switch(dx_state) 
		{

		/////////////////////////////////////////////////////
		case DX_PREP_REFRESH:
			for(i=0;i<4;++i) {
				dx_buf[i] = led_buf[i];
			}
			for(i=0;i<8;++i) {
				if(led_timeout[i]) {
					--led_timeout[i];
					if(led_timeout[i]) {
						dx_buf[3] |= (1<<i);
					}
					else {
						dx_buf[3] &= ~(1<<i);
					}
				}
			}
			dx_state = DX_REFRESH;
			// fall through
		
		/////////////////////////////////////////////////////
		case DX_REFRESH:
			// Load shift reg content to outputs, disabling
			// all sink lines and turning off the display
			P_SR_RCK = 0;
			P_SR_RCK = 1;	
		
			// Load shift register
			byte d = dx_buf[dx_disp];
			byte m = 0x80;
			while(m) {
				P_SR_SCK = 0;
				P_SR_DAT = !(d&m);
				P_SR_SCK = 1;
				m>>=1;
			}
			
			// Ensure all source lines are disabled
			P_SRC1 = 1;
			P_SRC2 = 1;
			P_SRC3 = 1;
			P_SRC4 = 1;
					
			// Now actually load the SR outputs and load the display
			// data to the LEDs
			P_SR_RCK = 0;
			P_SR_RCK = 1;	
			
			// fill the shift register with high bits ready to
			// disable all (active low) sink lines
			P_SR_SCK = 0; P_SR_SCK = 1;
			P_SR_SCK = 0; P_SR_SCK = 1;
			P_SR_SCK = 0; P_SR_SCK = 1;
			P_SR_SCK = 0; P_SR_SCK = 1;
			P_SR_SCK = 0; P_SR_SCK = 1;
			P_SR_SCK = 0; P_SR_SCK = 1;
			P_SR_SCK = 0; P_SR_SCK = 1;
			P_SR_SCK = 0; P_SR_SCK = 1;
			
			// enable the appropriate display source driver. At this point
			// all SR outputs should be HIGH
			switch(dx_disp) {		
				case 0: P_SRC1 = 0; break;
				case 1: P_SRC2 = 0; break;
				case 2: P_SRC3 = 0; break;
				case 3: P_SRC4 = 0; break;
			}
			
			// prepare for next display
			if(++dx_disp >= 4) {
				dx_state = DX_PREP_KEYSCAN;
				dx_disp = 0; 
			}
			// set next timer interrupt after display on time
			//****
			tmr1h = TIMER_INIT_DISPLAY_HOLD;
			break;
					
		/////////////////////////////////////////////////////
		case DX_PREP_KEYSCAN:
			// Load shift reg content to outputs, disabling
			// all sink lines and turning off the display
			P_SR_RCK = 0;
			P_SR_RCK = 1;	
		
			// clear key status buffer
			dx_key_state = 0;
			wpua = 0b00001010;
		
			// Clock a single zero bit into shift register and set 
			// data line high again
			P_SR_DAT = 0;
			P_SR_SCK = 0;
			P_SR_SCK = 1;	
			P_SR_DAT = 1;
		
			// copy shift reg to store reg
			P_SR_RCK=0; P_SR_RCK=1;
	
			// prepare to scan keys
			dx_mask = 0x80;
			dx_state = DX_KEYSCAN;
			
			// next tick after key settle time
			//****
			tmr1h = TIMER_INIT_KEY_SETTLE;
			break;
			
		/////////////////////////////////////////////////////
		case DX_KEYSCAN:	
		
			// check the scan line
			if(!P_KB_READ) {
				dx_key_state |= dx_mask;
			}
			
			// shift enable bit along
			P_SR_SCK = 0; P_SR_SCK = 1;			
			
			// present at outputs
			P_SR_RCK=0; P_SR_RCK=1;
			
			// move the scan bit along
			dx_mask >>= 1;
			
			if(!dx_mask) {
				// scan is complete, last job is to check 
				// the direct;y read switch
				if(!P_SWITCH) {
					dx_key_state |= 0x20;
				}
				
				// turn off pull ups
				wpua = 0b00000000;
				
				// back to display refresh mode
				key_state = dx_key_state;
				dx_state = DX_PREP_REFRESH;
			}
			// next tick after key settle time
			//****
			tmr1h = TIMER_INIT_KEY_SETTLE;
			break;
		}	
		
	}
		
	// serial rx ISR
	if(pir1.5)
	{	
		// get the byte
		byte b = rcreg;
		
		// calculate next buffer head
		byte next_rx_head = (rx_head + 1) & RX_BUFFER_MASK;
		
		// if buffer is not full
		if(next_rx_head != rx_tail)
		{
			// store the byte
			rx_buffer[rx_head] = b;
			rx_head = next_rx_head;
		}		
//		ui_blink_led(LED_ACT);
	}
}



// RA2 / CCP3 / P_PWM3
// RC3 / CCP2 / P_PWM1
// RC5 / CCP1 / P_PWM4
// RC6 / CCP4 / P_PWM2




////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void init_usart()
{
	pir1.1 = 1;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 0;		//TXIE 		no interrupts
	pie1.5 = 1;		//RCIE 		enable
	
	baudcon.4 = 0;	// SCKP		synchronous bit polarity 
	baudcon.3 = 1;	// BRG16	enable 16 bit brg
	baudcon.1 = 0;	// WUE		wake up enable off
	baudcon.0 = 0;	// ABDEN	auto baud detect
		
	txsta.6 = 0;	// TX9		8 bit transmission
	txsta.5 = 0;	// TXEN		transmit enable
	txsta.4 = 0;	// SYNC		async mode
	txsta.3 = 0;	// SEDNB	break character
	txsta.2 = 0;	// BRGH		high baudrate 
	txsta.0 = 0;	// TX9D		bit 9

	rcsta.7 = 1;	// SPEN 	serial port enable
	rcsta.6 = 0;	// RX9 		8 bit operation
	rcsta.5 = 1;	// SREN 	enable receiver
	rcsta.4 = 1;	// CREN 	continuous receive enable
		
	spbrgh = 0;		// brg high byte
	spbrg = 63;		// brg low byte (31250)		
	
}

byte clamp(byte d, byte min, byte max) {
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

void set_run_mode(int mode) 
{
	if(mode >= 0 && mode < RUN_MODE_MAXENUM)
	{
		switch(mode)
		{
		case RUN_MODE_TRIG_MIX:
		case RUN_MODE_TOGGLE_MIX:
		case RUN_MODE_TRIGCV_MIX:
		case RUN_MODE_SOLOTRIG_MIX:
		case RUN_MODE_VELTRIG_MIX:
			chan_mixer_mode = 1;
			break;
		default:
			chan_mixer_mode = 0;
			break;
		}
		trig_chan = 0;
		run_mode = mode;
	}
}
int get_run_mode()
{
	return run_mode;
}

////////////////////////////////////////////////////////////
// INTERRUPT HANDLER 
void init_display() {
	// set all source lines high to turn off display
	// (also sets SR input line high)
	P_SRC1 = 1; 
	P_SRC2 = 1; 
	P_SRC3 = 1; 
	P_SRC4 = 1; //P_SR_DAT = 1;
	
	// clock in eight HIGH bits to shift reg, to turn off 
	// all (active LOW) outputs
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	
	// and load data to SR output pins
	P_SR_RCK = 0;
	P_SR_RCK = 1;		

	dx_state = DX_PREP_REFRESH;
	dx_disp = 0;
	dx_mask = 0;
	dx_key_state = 0;
}

////////////////////////////////////////////////////////////////
// RUN ONE SHOT MODE
void run_oneshot(byte toggle) {
	// edge detection on CV inputs
	for(byte i=0; i<CHAN_MAX; ++i) {
		byte *cv_in = &adc_cv_state[i];
		if(*cv_in & ADC_CV_RISING_EDGE) {
			*cv_in &= ~ADC_CV_RISING_EDGE;
			if(toggle) {
				chan_toggle(i);
			}
			else {
				chan_trig(i);
			}
		}
		else if(*cv_in & ADC_CV_FALLING_EDGE) {		
			*cv_in &= ~ADC_CV_FALLING_EDGE;
			// handle an untrigger
			if(!toggle) {
				chan_untrig(i);
			}
		}	
	}
}

////////////////////////////////////////////////////////////////
// 
void run_oneshot_step() {
	byte *cv_in; 
	byte i;
	cv_in = &adc_cv_state[1];
	if(*cv_in & ADC_CV_RISING_EDGE) {
		*cv_in &= ~ADC_CV_RISING_EDGE;		
		for(i=0; i<4; ++i) {
			chan_reset_cycle(i);
		}
	}
	
	cv_in = &adc_cv_state[0];
	if(*cv_in & ADC_CV_RISING_EDGE) {
		*cv_in &= ~ADC_CV_RISING_EDGE;
		ui_blink_led(4);
		for(i=0; i<4; ++i) {
			chan_ping(i);
		}
	}
}

////////////////////////////////////////////////////////////////
// VELOCITY TRIG
void run_oneshot_vel() {
	for(byte i=0; i<CHAN_MAX; ++i) {
		if(adc_cv_state[i] & ADC_CV_RESULT) {
			adc_cv_state[i] &= ~ADC_CV_RESULT;
			chan_push_env(i, adc_cv_result[i]<<6);
		}
	}
}

////////////////////////////////////////////////////////////////
// DIRECT FADER MODE
void run_faders_cv() {
	for(byte i=0; i<CHAN_MAX; ++i) {
		if(adc_cv_state[i] & ADC_CV_RESULT) {
			adc_cv_state[i] &= ~ADC_CV_RESULT;
			chan_vca(i, adc_cv_result[i]<<6);
		}
	}
}

////////////////////////////////////////////////////////////////
// RUN CROSSFADE MODE
void run_crossfade_cv() {
	if(adc_cv_state[0] & ADC_CV_RESULT) {
		adc_cv_state[0] &= ~ADC_CV_RESULT;
		byte vca[4];
		vca[0] = 0;
		vca[1] = 0;
		vca[2] = 0;
		vca[3] = 0;
		byte x = (byte)adc_cv_result[0];	
		if(adc_cv_result[0] < 128) {
			vca[0] = 255;			
		}
		else if(adc_cv_result[0] < 384) {
			vca[0] = 384-x;			
			vca[1] = x - 128;			
		}
		else if(adc_cv_result[0] < 640) {
			vca[1] = 384-x;			
			vca[2] = x - 128;			
		}
		else if(adc_cv_result[0] < 896) {
			vca[2] = 384-x;			
			vca[3] = x - 128;			
		}
		else {
			vca[3] = 255;			
		}
		chan_vca(0, ((word)vca[0])<<8);
		chan_vca(1, ((word)vca[1])<<8);
		chan_vca(2, ((word)vca[2])<<8);
		chan_vca(3, ((word)vca[3])<<8);
	}
}

////////////////////////////////////////////////////////////////
// RUN CV ONESHOT MODE
void run_oneshot_cv(byte *cur_chan) {
	if(adc_cv_state[0] & ADC_CV_RESULT) {
		adc_cv_state[0] &= ~ADC_CV_RESULT;
		
		byte new_chan;
		if(adc_cv_result[0] < 256) {
			new_chan = 0;
		}
		else if(adc_cv_result[0] < 512) {
			new_chan = 1;
		}
		else if(adc_cv_result[0] < 768) {
			new_chan = 2;
		}
		else {
			new_chan = 3;
		}
		
		if(new_chan != *cur_chan) {
			if(*cur_chan < 4) {
				chan_untrig(*cur_chan);
			}
			chan_trig(new_chan);
			*cur_chan = new_chan;
		}
	}
}

////////////////////////////////////////////////////////////////
// PRIORITY SOLO
void run_solo(byte *cur_chan) {
	byte new_chan = 0xFF;
	for(byte i=0; i<CHAN_MAX; ++i) {
		if(adc_cv_state[i] & ADC_CV_HIGH) {
			new_chan = i;
		}
	}
	if(new_chan != 0xFF && new_chan != *cur_chan) {
		if(*cur_chan < 4) {
			chan_untrig(*cur_chan);
		}
		chan_trig(new_chan);
		*cur_chan = new_chan;
	}
}

////////////////////////////////////////////////////////////
// GET MESSAGES FROM MIDI INPUT
byte midi_in()
{
	// loop until there is no more data or
	// we receive a full message
	for(;;)
	{
		// usart buffer overrun error?
		if(rcsta.1)
		{
			rcsta.4 = 0;
			rcsta.4 = 1;
		}
		
		// check for empty receive buffer
		if(rx_head == rx_tail)
			return 0;
		
		// read the character out of buffer
		byte ch = rx_buffer[rx_tail];
		++rx_tail;
		rx_tail&=RX_BUFFER_MASK;

		// REALTIME MESSAGE
		if((ch & 0xf0) == 0xf0)
		{
		}    
		// STATUS BYTE
		else if(!!(ch & 0x80))
		{
			midi_param = 0;
			midi_status = ch; 
			switch(ch & 0xF0)
			{
			case 0xA0: //  Aftertouch  1  key  touch  
			case 0xC0: //  Patch change  1  instrument #   
			case 0xD0: //  Channel Pressure  1  pressure  
				midi_num_params = 1;
				break;    
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			default:
				midi_num_params = 2;
				break;        
			}
		}    
		else 
		{
			if(midi_status)
			{
				// gathering parameters
				midi_params[midi_param++] = ch;
				if(midi_param >= midi_num_params)
				{
					// we have a complete message.. is it one we care about?
					midi_param = 0;
					switch(midi_status&0xF0)
					{
					case 0x80: // note off
					case 0x90: // note on
					case 0xE0: // pitch bend
					case 0xB0: // cc
					case 0xD0: // aftertouch
						return midi_status; 
					}
				}
			}
		}
	}
	// no message ready yet
	return 0;
}

////////////////////////////////////////////////////////////
// MAIN
void main()
{ 
	int i;
	
	// set to 32MHz clock (also requires specific CONFIG1 and CONFIG2 settings)
	osccon = 0b11110000;

	trisa = TRIS_A;
	trisb = TRIS_B;
	trisc = TRIS_C;

	apfcon0.7 = 0; // RX/DT function is on RB5
	apfcon1.0 = 0; // CCP2SEL (CCP2 function is on RC3)
	
	ansela = DEF_ANSELA;
	anselb = DEF_ANSELB;
	anselc = DEF_ANSELC;
		
	P_SRC1		= 1;
	P_SRC2		= 1;
	P_SRC3		= 1;
	P_SRC4		= 1;
	
	P_PWM1		= 1;
	P_PWM2		= 1;
	P_PWM3		= 1;
	P_PWM4		= 1;

	//porta = 0;
	//portb = 0;
	//portc = 0;
	
	wpua = 0b00000000;
	wpub = 0b00000000;
	wpuc = 0b00000000;
	option_reg.7 = 0; // weak pull up enable

	// ensure the output drivers for each
	// of the CCPx outputs are disabled
	TRIS_PWM1 = 1;
	TRIS_PWM2 = 1;
	TRIS_PWM3 = 1;
	TRIS_PWM4 = 1;
			
	// Set to standard PWM mode
	ccp1con = 0b00001100; 
	ccp2con = 0b00001100; 
	ccp3con = 0b00001100; 
	ccp4con = 0b00001100; 

	// zero all duty cycles
	ccpr1l = 0; 
	ccpr2l = 0; 
	ccpr3l = 0; 
	ccpr4l = 0; 
	
	// set each CCP module to use timer 2
	ccptmrs = 0b00000000;
	
	// Configure timer 2 for x1 prescaler
	t2con = 0b00000000;

	// load timer 2 period register for 255 duty cycles	
	pr2 = 0xFF; 
	
	
	// clear Timer2 interrupt flag
	pir1.1 = 0;
	
	// start up the timer
	t2con.2 = 1;	
	
	// wait for Timer2 to overflow once
	while(!pir1.1); 


	// now we can enable the output drivers
	TRIS_PWM1 = 0;
	TRIS_PWM2 = 0;
	TRIS_PWM3 = 0;
	TRIS_PWM4 = 0;

	adcon1.7  = 1; // right justify result
	adcon1.6 = 1; // } clock select for ADC
	adcon1.5 = 0; // } Fosc/16
	adcon1.4 = 1; // }
	adcon1.2 = 0; // VSS is lower ref
	adcon1.1 = 0; // } VDD is upper ref
	adcon1.0 = 0; // } 

	adcon0.0 = 1; // ADC on
	
		
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 8MHz
	// 	prescaled 1/32 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	option_reg.3 = 0; // timer 0 is prescaled
	option_reg.2 = 1; // }
	option_reg.1 = 0; // } 1/32 prescaler
	option_reg.0 = 0; // }
	intcon.5 = 1; 	  // enabled timer 0 interrrupt
	intcon.2 = 0;     // clear interrupt fired flag
	
	
	// Configure timer 1 
	// instruction clock (8MHz, 8x prescaler = 1MHz)	
	t1con = 0b00000001;
	pie1.0 = 1; // enable timer1 overflow
	pir1.0 = 0; // clear interrupt flag
	
	init_usart();
	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE
		
		
	chan_init();
	init_display();


	// main app loop
	word d = 0;
	//byte q=0;

	set_run_mode(RUN_MODE_TRIG);
	byte last_key_state = 0;

	while(1) {
		byte msg = midi_in();
		if(msg) {
			ui_blink_led(LED_ACT);
		}
		
		adc_run();
		switch(run_mode) {
			case RUN_MODE_TRIG: 	
			case RUN_MODE_TRIG_MIX: 	
				run_oneshot(false); 
				break;
			case RUN_MODE_TOGGLE:	
			case RUN_MODE_TOGGLE_MIX:	
				run_oneshot(true); 
				break;
			case RUN_MODE_TRIGCV: 	
			case RUN_MODE_TRIGCV_MIX: 	
				run_oneshot_cv(&trig_chan); 
				break;
			case RUN_MODE_SOLOTRIG: 
			case RUN_MODE_SOLOTRIG_MIX: 
				run_solo(&trig_chan); 
				break;
			case RUN_MODE_VELTRIG:
			case RUN_MODE_VELTRIG_MIX:
				run_oneshot_vel();
				break;
			case RUN_MODE_FADERSCV:	
				run_faders_cv(); 
				break;
			case RUN_MODE_XFADECV:	
				run_crossfade_cv(); 
				break;
			case RUN_MODE_STEP:	
				run_oneshot_step(); 
				break;
		};
		
		if(tick_flag) {
			tick_flag = 0;
			chan_tick();
			ui_tick();
			//if(!q) {
			//	q=1000;
			//	chan_trig(3);
			//}
			//--q;
		}
	
		// check for any change to key statuses
		byte delta = key_state ^ last_key_state;		
		if(delta & key_state) { // Key is pressed
			ui_notify(delta & key_state, key_state);
		}
		last_key_state = key_state;
		
	}
}

//
// END
//

