#include <system.h>
#include "quadvca.h"
#define MAX_DUTY 1023
void vca_set(byte which, unsigned int level)
{
	level &= MAX_DUTY;
	level = MAX_DUTY - level;
	switch(which) {
		case VCA1:
			ccpr2l = level>>2;
			ccp2con.5 = level.1;
			ccp2con.4 = level.0;	
			break;
		case VCA2:
			ccpr4l = level>>2;
			ccp4con.5 = level.1;
			ccp4con.4 = level.0;	
			break;
		case VCA3:
			ccpr3l = level>>2;
			ccp3con.5 = level.1;
			ccp3con.4 = level.0;	
			break;
		case VCA4:
			ccpr1l = level>>2;
			ccp1con.5 = level.1;
			ccp1con.4 = level.0;	
			break;
	}
}
