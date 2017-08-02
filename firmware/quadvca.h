typedef unsigned char byte;

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



extern unsigned int adc_cv_result[4];
extern byte adc_cv_state[4];



void adc_run();
void vca_set(byte which, unsigned int level);


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
	byte mode;
	byte state;
	ENV_COUNTER value;	
	ENV_COUNTER attack; 
	ENV_COUNTER release; 	
} ENVELOPE;

typedef struct {
	ENVELOPE env;
} VCA_CHANNEL;

void chan_init(VCA_CHANNEL *chan);
void chan_run(byte which, VCA_CHANNEL *chan);

#define ENV_VALUE(env) ((ENV_COUNTER)(((env)->value) >> 6))
void env_init(ENVELOPE *env);
void env_run(ENVELOPE *env);
void env_trig(ENVELOPE *env);
void env_untrig(ENVELOPE *env);
