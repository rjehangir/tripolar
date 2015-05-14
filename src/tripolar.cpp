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
	motorPwm.set_pwm(bldcPwm::ePwmChannel_A,300);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_B,300);
	motorPwm.set_pwm(bldcPwm::ePwmChannel_C,300);
	motorPwm.update();
}



void loop(void)
{
	
	
	
}