/*
 * tripolar.cpp
 *
 * Created: 5/12/2015 4:02:05 PM
 *  Author: Alan
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "bldcPwm.h"




//#define DO_DEBUG
#include "afro_nfet.h"
#include "fets.h"
#define F_CPU 16000000UL
#include <util/delay.h>
#include "millis.h"

void setup(void);
void loop(void);
void incrementRotor(uint8_t value);
void set_rpm(uint16_t value);




	/*****************************************************************************/
	/**  Sine Wave Table. Given an index in degrees between 0 and 359, 
	 *	 outputs the sine normalized to a output of 0 to 255.
	 *
	 *  
	 *  @attention
	 *		The values at the top and bottom of the sine wave appear to be   	
	 *		drawn out from what you would normally expect to see in a  sine
     *      wave. When viewed on a scope, the pulse width seems to linger 
     *      at the top and bottom. This may have been intentional, or this 
     *      might be something we need to investigate.							*/
	/****************************************************************************/	
	//static const uint8_t pwmSin[2] = {0,1};
	
	//static const uint8_t pwmSin[256]  = 	    			 															
	static const uint8_t pwmSin[256]  = 	
	{
				128, 	131, 	134, 	137, 	140, 	143, 	146, 	149, 	152, 	156, 	//	0	 to 	9
				159, 	162, 	165, 	168, 	171, 	174, 	176, 	179, 	182, 	185, 	//	10	 to 	19
				188, 	191, 	193, 	196, 	199, 	201, 	204, 	206, 	209, 	211, 	//	20	 to 	29
				213, 	216, 	218, 	220, 	222, 	224, 	226, 	228, 	230, 	232, 	//	30	 to 	39
				234, 	236, 	237, 	239, 	240, 	242, 	243, 	245, 	246, 	247, 	//	40	 to 	49
				248, 	249, 	250, 	251, 	252, 	252, 	253, 	254, 	254, 	254, 	//	50	 to 	59
				254, 	254, 	254, 	254, 	254, 	254, 	254, 	254, 	254, 	254, 	//	60	 to 	69
				254, 	254, 	253, 	252, 	252, 	251, 	250, 	249, 	248, 	247, 	//	70	 to 	79
				246, 	245, 	243, 	242, 	240, 	239, 	237, 	236, 	234, 	232, 	//	80	 to 	89
				230, 	228, 	226, 	224, 	222, 	220, 	218, 	216, 	213, 	211, 	//	90	 to 	99
				209, 	206, 	204, 	201, 	199, 	196, 	193, 	191, 	188, 	185, 	//	100	 to 	109
				182, 	179, 	176, 	174, 	171, 	168, 	165, 	162, 	159, 	156, 	//	110	 to 	119
				152, 	149, 	146, 	143, 	140, 	137, 	134, 	131, 	128, 	124, 	//	120	 to 	129
				121, 	118, 	115, 	112, 	109, 	106, 	103, 	99, 	96, 	93, 	//	130	 to 	139
				90, 	87, 	84, 	81, 	79, 	76, 	73, 	70, 	67, 	64, 	//	140	 to 	149
				62, 	59, 	56, 	54, 	51, 	49, 	46, 	44, 	42, 	39, 	//	150	 to 	159
				37, 	35, 	33, 	31, 	29, 	27, 	25, 	23, 	21, 	19, 	//	160	 to 	169
				18, 	16, 	15, 	13, 	12, 	10, 	9, 		8,		7,		6, 		//	170	 to 	179
				5,		4,		3,		3,		2,		1,		1,		1, 		1,		1, 		//	180	 to 	189
				1, 		1,	 	1,		1,	 	1,	 	1,	 	1,		1,	 	1,		1, 		//	190	 to 	199
				2, 		3,		3,		4,	 	5,		6,		7,		8,		9,		10, 	//	200	 to 	209
				12, 	13, 	15, 	16, 	18, 	19, 	21, 	23, 	25, 	27, 	//	210	 to 	219
				29, 	31, 	33, 	35, 	37, 	39, 	42, 	44, 	46, 	49, 	//	220	 to 	229
				51, 	54, 	56, 	59, 	62, 	64, 	67, 	70, 	73, 	76, 	//	230	 to 	239
				79, 	81, 	84, 	87, 	90, 	93, 	96, 	99, 	103, 	106, 	//	240	 to 	249
				109, 	112, 	115, 	118, 	121, 	124										//	250	 to 	255		
	}; 
	
#define SINE_ARRAY_SIZE 255
#define PHASE_SHIFT  85

bldcPwm motorPwm;
uint16_t speed_rpm;


	
uint8_t baseIncrement = 0;  /*This is the amount which needs to be incremented every PWM cycle */
uint16_t incrementDelay_ms = 1000; /*The accumulator is incremented by one each time this amount of time passes*/

int main(void)
{
	boardInit();
	cli();
	redOff();
	greenOn();
	DEBUG_OUT(0x00);
	//_delay_ms(500);
	for (uint8_t n=0;n<=0x0F;n++)
	{
			DEBUG_OUT(n);
			//_delay_ms(10);
	}
	setup();
	
	
    while(1)
    {
        //TODO:: Please write your application code 
		loop();
    } 
}


void setup(void)
{
	millis_init();
	motorPwm.begin();
	set_rpm(500);	
}

void loop(void)
{
	static int16_t loopCount = 0;
	//static int16_t cycleCount = 0;
	//bool newValue =false;
	//static bool done = false;
	//static bool enabled = true;
	//static uint8_t rotorIncrement = 0;
	
	
	//static uint8_t incrementAmount = 1;
	
	
	
	static uint8_t accumulator = 0; /*This is an additional amount which is incremented via a timer and added to the base increment*/
	static uint32_t accelerateTimer; 
	static uint32_t incrementTimer = 0;
	
	loopCount++;
	
	motorPwm.tickle();
	
	
	if (_ms - incrementTimer >= incrementDelay_ms )
	{
		incrementTimer = _ms;
		accumulator++;
	}
	
	

	if (!motorPwm.busy())
	{
		incrementRotor(baseIncrement + accumulator);	
		accumulator = 0;	
	}

	
	DEBUG_OUT(0x05);
	
	
					
	
	
}


void set_rpm(uint16_t value)
{
	uint32_t numerator = PWM_INCREMENT_SCALER_NUMERATOR;
	numerator *= value;
	numerator *= 1000;
	uint32_t denomenator = 	PWM_INCREMENT_SCALER_DENOMENATOR;
	uint32_t calcValue = numerator / denomenator;
	baseIncrement = calcValue / 1000;
	uint32_t remainder = calcValue - ((uint32_t)baseIncrement *1000);
	incrementDelay_ms = 1000 / remainder;	
}


void incrementRotor(uint8_t value)
{
   static uint8_t currentStep = 0;	
   uint16_t pwmA,pwmB,pwmC;		
   uint8_t indexA,indexB,indexC;

   
   currentStep += value;
   
   indexA = currentStep;
   indexB = currentStep + PHASE_SHIFT;
   indexC = indexB + PHASE_SHIFT;
   
   
  // pwmA = pgm_read_byte_near(pwmSin + indexA) * 4;
  // pwmB = pgm_read_byte_near(pwmSin + indexB) * 4;
  // pwmC = pgm_read_byte_near(pwmSin + indexC) * 4;
  
  
   pwmA = pwmSin[indexA]*4;
   pwmB = pwmSin[indexB]*4;
   pwmC = pwmSin[indexC]*4;
  
	motorPwm.set_pwm(bldcPwm::ePwmChannel_A,pwmA);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_B,pwmB);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_C,pwmC);				
 	motorPwm.update();
};