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
#include <stdlib.h>


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTION PROTOTYPES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

void setup(void);
void loop(void);



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STRUCTURES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

/*****************************************************************************************************/
/* STRUCT: servoIsrData_S																			 */
/** Data structure used to interact with the Servo Interrupt service routine						 */
/*****************************************************************************************************/
typedef struct servoIsrData_S
{
																
	
	volatile uint16_t startTimeStamp;
	volatile uint16_t stopTimeStamp;
	bool dataReady;
	bool waitRising; //If true, we are waiting for a rising edge, otherwise a falling
	
}servoIsrData_T;   


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& VARIABLES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

bldcGimbal gimbal;
static servoIsrData_T servoIsrData;


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INTERRUPT SERVICE ROUTINES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

ISR(TIMER1_CAPT_vect)
{
	sei();
	volatile uint16_t timeStamp;	
	timeStamp = micros();
	if (servoIsrData.dataReady) return; //we already have a reading, so don't collect a new one till this one is read.
   
	if (servoIsrData.waitRising) {
		servoIsrData.startTimeStamp = timeStamp;  //If Rising Edge, record the start time						
		TCCR1B &= ~_BV(ICES1);//Set interrupt for falling edge
		servoIsrData.waitRising = false;
	}
	else  //Otherwise its a falling edge so ..	
	{
		servoIsrData.stopTimeStamp = timeStamp;	//Record the stop time..
		servoIsrData.dataReady = true;			//And signal that we have a value
		servoIsrData.waitRising = true;
		TCCR1B |= _BV(ICES1); //Set interrupt for rising edge				
	}
}



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& SERVO ISR SUPPORT FUNCTIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/


inline void servo_begin(void)
{
	sei(); //Enable nested interrupts
	servoIsrData.dataReady = false;
	servoIsrData.waitRising = true;
	
	//Configure PCINT0 PIN (PB0) as an input
	DDRB &= ~_BV(DDB0);
	
	//DISABLE PULLUP RESISTOR (PB0)
	PORTB &= ~_BV(PORTB0);
	
	//Rising Edge Detect
	TCCR1B |= _BV(ICES1);
	
	//Enable Input Compare Interrupt
	TIMSK |= _BV(TICIE1);
	
	
}



inline bool servo_changed(void)
{
	if (servoIsrData.dataReady)
	{		
		if (gimbal.motorPwm().icr1Conflict()) {
			TIMSK &= ~_BV(TICIE1);
			servoIsrData.dataReady = false;		
			TIMSK |= _BV(TICIE1);
			redOn();				
		}
	}						 	
	return servoIsrData.dataReady;	
}


inline uint16_t servo_value_uS(void)
{
	TIMSK &= ~_BV(TICIE1);
	uint16_t retVal = servoIsrData.stopTimeStamp - servoIsrData.startTimeStamp;	
	TIMSK |= _BV(TICIE1);
	redOff();	
	servoIsrData.dataReady = false;
	
	return retVal;
}


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
	servo_begin();
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
	static int16_t currentSpeed = 0;
	static int16_t lastServo = 0;
	static int16_t lastServo2 = 0;
	int16_t currentServo;
	static int16_t averageSpeed = 0;
	
	
	if (servo_changed()) 
	{
		
		currentServo = servo_value_uS();	
		
		if(!(abs(currentServo-lastServo)>25 && (abs(lastServo - lastServo2) <= 50 )))
		{
			
		
			if (abs(currentServo-1500)>1)
			{
				asm("NOP");
			}	
			if (currentServo != lastServo)
			{
				if (currentServo < 2200 && currentServo > 800)								
				{
					#define DEADZONE 5
					currentSpeed = currentServo - 1500;
					int8_t sign = (currentSpeed<0?-1*DEADZONE:DEADZONE);				
					currentSpeed = (abs(currentSpeed)<=DEADZONE?0:(11*((currentSpeed-sign)))/10);
					averageSpeed = ((averageSpeed*7) + (currentSpeed*3))/10;					
					uint16_t powerScale1 = 0 + (5*abs(currentSpeed));  //100/10 = 2/1
					uint16_t powerScale2 = 50 + (5*abs(currentSpeed) /30);// 50/300 = 5/40
					//uint8_t powerScale = 4;
				
					gimbal.set_PowerScale(powerScale1>powerScale2?powerScale2:powerScale1);
				
			 		gimbal.set_speed_rpm(currentSpeed);		
					//gimbal.set_speed_rpm(currentSpeed);		
				}
			}
		} // END if (!abs(currentServo-lastServo)>100)
		lastServo = currentServo;
		lastServo2 = lastServo;
	} //END 	if (servo_changed()) 
						
	gimbal.tickle();	
}


