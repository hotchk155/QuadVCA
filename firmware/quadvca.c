#include <system.h>
#include <memory.h>
#include <eeprom.h>


// CONFIG OPTIONS 
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_ON & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000

#include "quadvca.h"

/*
		
	
PIC PWM RESOURCES


RA2	/ CCP3
RC3 / CCP2
RC5 / CCP1
RC6 / CCP4


		
*/

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



                        //76543210
#define DEF_ANSELA		0b00000000
#define DEF_ANSELB		0b00010000
#define DEF_ANSELC		0b00000111


					//76543210
#define TRIS_A		0b11001111
#define TRIS_B		0b00111111
#define TRIS_C		0b01101111


#define P_KB_READ		porta.1

//
// TYPE DEFS
//


//
// GLOBAL DATA
//
VCA_CHANNEL g_chan[4];

unsigned int vca_level[4] = {0};

volatile byte tick_flag = 0;
#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz
									// using this init scalar means that rollover
									// interrupt fires once per ms


////////////////////////////////////////////////////////////
// INTERRUPT HANDLER CALLED WHEN CHARACTER RECEIVED AT 
// SERIAL PORT OR WHEN TIMER 1 OVERLOWS
void interrupt( void )
{
	// timer 0 rollover ISR. Maintains the count of 
	// "system ticks" that we use for key debounce etc
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		tick_flag = 1;
		intcon.2 = 0;
	}

	// timer 1 rollover ISR. Responsible for timing
	// the tempo of the MIDI clock
/*	if(pir1.0)
	{
		tmr1l=(timer_init_scalar & 0xff); 
		tmr1h=(timer_init_scalar>>8); 
		tick_flag = 1;
		pir1.0 = 0;
	}*/
		
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




// when called, the shift and store registers will be filled with 1's
byte key_state = 0;
void scan_keys()
{
	key_state = 0;
	wpua = 0b00000010;
	
	// Clock a single zero bit into shift register and set 
	// data line high again
	P_SR_DAT = 0;
	P_SR_SCK = 0;
	P_SR_SCK = 1;	
	P_SR_DAT = 1;
	
	byte m = 0x80;
	while(m) {
	
		// copy shift reg to store reg
		P_SR_RCK=0; P_SR_RCK=1;

delay_us(100);				
		if(!P_KB_READ) {
			key_state |= m;
		}
		
		// shift enable bit along
		P_SR_SCK = 0; P_SR_SCK = 1;			
		m>>=1;
	}
	
	// now shift reg is cleared, move to store reg
	P_SR_RCK=0; P_SR_RCK=1;
	wpua = 0b00000000;
	
}

void load_sr(byte d)
{

	// Load shift register
	byte m = 0x80;
	while(m) {
		P_SR_SCK = 0;
		P_SR_DAT = !!(d&m);
		P_SR_SCK = 1;
		m>>=1;
	}
	
	// Set data line high (which also disables the diplay source 
	// sharing enable line with the data line)
	P_SR_DAT = 1;
	// enable the appropriate display souce
	P_SRC1 = 0;
	
	// Load shift reg content to outputs
	P_SR_RCK = 0;
	P_SR_RCK = 1;	


	// fill the shift register with high bits
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	P_SR_SCK = 0; P_SR_SCK = 1;
	
	// 
	delay_ms(2);
	
	// Load shift reg content to outputs
	P_SR_RCK = 0;
	P_SR_RCK = 1;	
	
	// Turn off all display
	P_SRC1 = 1;
	P_SRC2 = 1;
	P_SRC3 = 1;
	P_SRC4 = 1;
	
}

/*
	 AAA
    F   B
     GGG     
    E   C
     DDD
*/
enum {
	SEG_A  = 0x10,
	SEG_B  = 0x80,
	SEG_C  = 0x02,
	SEG_D  = 0x20,
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
	CHAR_9	= SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G
};
byte led_seg1=CHAR_9;
byte led_seg2=0;
byte led_seg3=0;

/*
	SR data line shared with a SRC enable line
	
	when SR is being loaded, output content must be 0xFF
	
*/
void refresh_display() {
	load_sr(~led_seg1);
}


void init() {
	for(byte i=0; i<4; ++i) {
		chan_init(&g_chan[i]);
	}
}
////////////////////////////////////////////////////////////
// MAIN
void main()
{ 
	int i;
	
/*	
The Internal Oscillator Block can be used with the 4X
PLL associated with the External Oscillator Block to
produce a 32 MHz internal system clock source. The
following settings are required to use the 32 MHz internal
clock source:
• The FOSC bits in Configuration Word 1 must be
set to use the INTOSC source as the device system
clock (FOSC<2:0> = 100).
• The SCS bits in the OSCCON register must be
cleared to use the clock determined by
FOSC<2:0> in Configuration Word 1
(SCS<1:0> = 00).
• The IRCF bits in the OSCCON register must be
set to the 8 MHz HFINTOSC set to use
(IRCF<3:0> = 1110).
• The SPLLEN bit in the OSCCON register must be
set to enable the 4xPLL, or the PLLEN bit of the
Configuration Word 2 must be programmed to a
‘1’.
The 4xPLL is not available for use with the internal
oscillator when the SCS bits of the OSCCON register
are set to ‘1x’. The SCS bits must be set	
*/
	
	// set to 32MHz clock (also requires specific CONFIG1 and CONFIG2 settings)
	osccon = 0b11110000;

        // osc control / 16MHz / internal
        //osccon = 0b01111010;
		
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
	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE
		
		
	init();
		
	// main app loop
	unsigned int d = 0;
	byte q=0;
	
	
	while(1) {
//		if(++d>1023) d=0;
//		set_vca(VCA4,d);
	//	set_vca(1,d);
	//	set_vca(2,d);
	//	set_vca(3,d);
//		delay_ms(1);
	
//		refresh_display();
//		scan_keys();
//		led_seg1 = key_state;
		//delay_us(250);
		//run_adc();
		//tick_flag = 0;
		//while(!tick_flag);
		
		adc_run();
		//set_vca(0, adc_result[0]);
		
		if(tick_flag) {
			tick_flag = 0;
			//	chan_run(0, &g_chan[0]);
			for(byte i=0; i<4; ++i) {
				chan_run(i, &g_chan[i]);
			}
		}
	}
}

//
// END
//