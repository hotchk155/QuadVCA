#include <system.h>
#include "quadvca.h"


////////////////////////////////////////////////////////////////
// STORE A PARAMETER
void global_set(byte param, int value) {
	switch(param) {
		case GLOBAL_SHARE_ENV:			
			chan_mixer_mode = clamp(value,0,1);
	}	
}

////////////////////////////////////////////////////////////////
// GET A PARAMETER
int global_get(byte param) {
	switch(param) {
		case GLOBAL_SHARE_ENV:			
			return chan_mixer_mode;
	}	
	return 0;
};