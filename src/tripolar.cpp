/*
 * tripolar.cpp
 *
 * Created: 5/12/2015 4:02:05 PM
 *  Author: Alan
 */ 



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& USER SETUP
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/

	#define F_CPU 16000000UL
	//#define DO_DEBUG
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INCLUDES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

#include <avr/io.h>
#include "bldcGimbal.h"

#include "afro_nfet.h"
#include "fets.h"

#include <util/delay.h>
#include "millis.h"


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTION PROTOTYPES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

void setup(void);
void loop(void);


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& VARIABLES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

bldcGimbal gimbal;



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

int main(void)
{	
	setup();		
    while(1) loop();    
}


void setup(void)
{
	
	boardInit();
	cli();
	redOff();
	greenOn();
	millis_init();
	gimbal.begin();
}


typedef enum accState_E
{
	eAccState_START,
	eAccState_INITIAL_RAMP,
	eAccState_HOLD,
	eAccState_NEXT_SPEED	
}accState_T;

void loop(void)
{
	static uint32_t lastTime = 0;
	static uint16_t currentSpeed = 0;
	static accState_T state = eAccState_START;
	static bool doRamp = false;
	static uint32_t stateTimer = 0;
	static uint16_t speedTarget = 0;
	
	
	
	switch (state)
	{
		case eAccState_START:
			stateTimer = _100us;
			state = eAccState_INITIAL_RAMP;
			doRamp = false;
			break;
		case eAccState_INITIAL_RAMP:
			doRamp = true;
			if (currentSpeed >= 200) {
				stateTimer = _100us;
				state = eAccState_HOLD;
			}
			break;
		case eAccState_HOLD:			
			doRamp = false;
			if (_100us - stateTimer >= 150000) {			//15  seconds
				stateTimer = _100us;
				state = eAccState_NEXT_SPEED;	
				speedTarget = currentSpeed + 50;
			}
			break;
			
		case eAccState_NEXT_SPEED:
			doRamp = true;
			if (speedTarget == currentSpeed) {
				state = eAccState_HOLD;
				stateTimer = _100us;
			}
			break;
		default:			
			doRamp = false;	
	}
			
	if (_100us-lastTime >=400 && doRamp)
	{
		lastTime = _100us;
		currentSpeed++;		
		gimbal.set_speed_rpm(currentSpeed);					
	}
		
	gimbal.tickle();	
}

