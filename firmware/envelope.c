
#include "quadvca.h"
void env_init(ENVELOPE *env) {
	env->mode = ENV_MODE_AR;
	env->state = ENV_ST_IDLE;
	env->value = 0;
	env->attack = 0xFFFF;
	env->release = 0x0080;
} 



void env_run(ENVELOPE *env) { // per ms processing
	long l;
	switch(env->state) {
		case ENV_ST_ATTACK:
			l = (long)env->value + env->attack;
			if(l >= 0xFFFF) {
				env->value = 0xFFFF;
				env->state = ENV_ST_RELEASE;
			}
			else {
				env->value = (ENV_COUNTER)l;
			}
			break;
		case ENV_ST_RELEASE:
			l = (long)env->value - env->release;
			if(l <= 0) {
				env->value = 0;
				env->state = ENV_ST_IDLE;
			}
			else {
				env->value = (ENV_COUNTER)l;
			}
			break;
		case ENV_ST_IDLE:
		case ENV_ST_SUSTAIN:
			break;
	}
}

void env_trig(ENVELOPE *env) {
	switch(env->state) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
			env->state = ENV_ST_ATTACK;
			break;
		case ENV_ST_ATTACK:
		case ENV_ST_SUSTAIN:
			break;
	}
}

void env_untrig(ENVELOPE *env) {
	switch(env->state) {
		case ENV_ST_IDLE:
		case ENV_ST_RELEASE:
		case ENV_ST_ATTACK:
			break;
		case ENV_ST_SUSTAIN:
			env->state = ENV_ST_ATTACK;
			break;
	}
}
