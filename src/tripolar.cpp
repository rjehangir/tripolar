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
	//sei(); //Enable nested interrupts
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
	return servoIsrData.dataReady;	
}


inline uint16_t servo_value_uS(void)
{
	uint16_t retVal = servoIsrData.stopTimeStamp - servoIsrData.startTimeStamp;
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
	static uint16_t lastServo = 0;
	uint16_t currentServo;
	static int16_t averageSpeed = 0;
	
	
	if (servo_changed()) 
	{
		currentServo = servo_value_uS();	
		if ((currentServo-1500)>0)
		{
			asm("NOP");
		}	
		if (currentServo != lastServo)
		{
			if (currentServo < 2000 && currentServo > 1000) 
			{
				currentSpeed = (currentServo - 1500)*2;
				averageSpeed = ((averageSpeed*9) + (currentSpeed*1))/10;
				lastServo = currentServo;
			 	//gimbal.set_speed_rpm(currentSpeed);		
				gimbal.set_speed_rpm(averageSpeed);		
			}
		}
	}
						
	gimbal.tickle();	
}


