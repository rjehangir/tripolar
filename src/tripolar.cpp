/*
 * tripolar.cpp
 *
 * Created: 5/12/2015 4:02:05 PM
 *  Author: Alan
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include "bldcPwm.h"

void setup(void);
void loop(void);



bldcPwm motorPwm;

int main(void)
{
	setup();
    while(1)
    {
        //TODO:: Please write your application code 
		loop();
    }
}



void setup(void)
{
	motorPwm.begin();
	motorPwm.set_pwm(bldcPwm::ePwmChannel_A,900);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_B,600);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_C,300);
	motorPwm.update();
}



void loop(void)
{
	static int16_t loopCount = 0;
	loopCount++;
	
	motorPwm.tickle();
	
	if (loopCount >=100)
	{
		incrementRotor();
		loopCount = 0;	
	}
	
	
}






void incrementRotor(void)
{
	static uint8_t currentStep = 0;	
   uint8_t pwmA,pwmB,pwmC;		
   uint8_t indexA,indexB,indexC;
   
   indexA = currentStep;
   indexB = currentStep + PHASE_SHIFT;
   indexC = indexB + PHASE_SHIFT;
   
   
   pwmA = pgm_read_byte_near(pwmSin + indexA);
   pwmB = pgm_read_byte_near(pwmSin + indexB);
   pwmC = pgm_read_byte_near(pwmSin + indexC);
   
   	
	motorPwm.set_pwm(bldcPwm::ePwmChannel_A,pwmA);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_B,pwmB);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_C,pwmC);				
 	motorPwm.update();
};