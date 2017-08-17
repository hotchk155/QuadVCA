////////////////////////////////////////////////////////////////
//

//
// INCLUDE FILES
//
#include <system.h>
#include <memory.h>
#include <eeprom.h>
#include "quadvca.h"

// CONFIG OPTIONS 
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_ON & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000



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
byte led_buf[4] = {0};

//
// FILE SCOPE DATA
//
#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz
									// using this init scalar means that rollover
									// interrupt fires once per ms
static volatile byte tick_flag = 0; // flag set when millsecond interrupt fires


// definitions for the display update state machine
#define DX_REFRESH_DISPLAY 			0
#define DX_BEGIN_KEYSCAN			1
#define DX_KEYSCAN			 		2
#define TIMER_INIT_DISPLAY_HOLD 	200	// display hold time (255 - timer1 ticks)
#define TIMER_INIT_KEY_SETTLE		254 // key input settle time (255 - timer1 ticks)
static volatile byte dx_state;
static volatile byte dx_disp;
static volatile byte dx_mask;
static volatile byte dx_key_state;
static volatile byte key_state = 0;



////////////////////////////////////////////////////////////
// INTERRUPT HANDLER 
void interrupt( void )
{
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
		case DX_REFRESH_DISPLAY:
			// Load shift reg content to outputs, disabling
			// all sink lines and turning off the display
			P_SR_RCK = 0;
			P_SR_RCK = 1;	
		
			// Load shift register
			byte d = led_buf[dx_disp];
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
				dx_state = DX_BEGIN_KEYSCAN;
				dx_disp = 0; 
			}
			// set next timer interrupt after display on time
			//****
			tmr1h = TIMER_INIT_DISPLAY_HOLD;
			break;
					
		/////////////////////////////////////////////////////
		case DX_BEGIN_KEYSCAN:
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
				dx_state = DX_REFRESH_DISPLAY;
			}
			// next tick after key settle time
			//****
			tmr1h = TIMER_INIT_KEY_SETTLE;
			break;
		}	
		
	}
		
/*	// serial rx ISR
	if(pir1.5)
	{	
		// get the byte
		byte b = rcreg;
		
		// calculate next buffer head
		byte nextHead = (rxHead + 1);
		if(nextHead >= SZ_RXBUFFER) 
		{
			nextHead -= SZ_RXBUFFER;
		}
		
		// if buffer is not full
		if(nextHead != rxTail)
		{
			// store the byte
			rxBuffer[rxHead] = b;
			rxHead = nextHead;
		}		
	}*/
}




////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
/*
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
	spbrg = 31;		// brg low byte (31250)		
	
}
*/


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

	dx_state = DX_REFRESH_DISPLAY;
	dx_disp = 0;
	dx_mask = 0;
	dx_key_state = 0;
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
	
	wpua = 0b00000000;
	option_reg.7 = 0; // weak pull up enable

	// enable interrupts	
	//intcon.7 = 1; //GIE
	//intcon.6 = 1; //PEIE	

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
	
	vca_set(VCA1,VCA_CLOSED);
	vca_set(VCA2,VCA_CLOSED);
	vca_set(VCA3,VCA_CLOSED);
	vca_set(VCA4,VCA_CLOSED);
	
	
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
	
	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE
		
		
	chan_init(0);
	chan_init(1);
	chan_init(2);
	chan_init(3);
	
	init_display();
		
	// main app loop
	unsigned int d = 0;
	byte q=0;

	byte last_key_state = 0;
	
	while(1) {
		adc_run();
		if(tick_flag) {
			tick_flag = 0;
			for(byte i=0; i<4; ++i) {
				chan_run(i);
			}
			ui_run();
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