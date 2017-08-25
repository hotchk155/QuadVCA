#include <system.h>
#include "quadvca.h"


////////////////////////////////////////////////////////////
// Control VCA 
// level is 10 bits (0-1023). 0 is off, 1023 is max
void vca(byte which, word level)
{
	if(level < 256) {
		level  = (level * 5)/2;
	}
	else {
		level = 512+(level/2);
	}
	level = 1023 - level;
	switch(which) {
		case 0:
			if(level>=1023) {
				ccp2con = 0b00000000; 
				P_PWM1 = 1;
			}
			else {
				ccpr2l = level>>2;
				ccp2con.5 = level.1;
				ccp2con.4 = level.0;	
				ccp2con = 0b00001100; 
			}				
			break;
		case 1:
			if(level>=1023) {
				ccp4con = 0b00000000; 
				P_PWM2 = 1;
			}
			else {
				ccpr4l = level>>2;
				ccp4con.5 = level.1;
				ccp4con.4 = level.0;	
				ccp4con = 0b00001100; 
			}				
			break;
		case 2:
			if(level>=1023) {
				ccp3con = 0b00000000; 
				P_PWM3 = 1;
			}
			else {
				ccpr3l = level>>2;
				ccp3con.5 = level.1;
				ccp3con.4 = level.0;	
				ccp3con = 0b00001100; 
			}				
			break;
		case 3:
			if(level>=1023) {
				ccp1con = 0b00000000; 
				P_PWM4 = 1;
			}
			else {
				ccpr1l = level>>2;
				ccp1con.5 = level.1;
				ccp1con.4 = level.0;	
				ccp1con = 0b00001100; 
			}				
			break;
	}
}

