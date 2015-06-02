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

ISR(INT0_vect)
{
	volatile uint16_t timeStamp;
	timeStamp = micros();
	if (servoIsrData.dataReady) return; //we already have a reading, so don't collect a new one till this one is read.
   
	if ((PINB & _BV(PINB0)) != 0) servoIsrData.startTimeStamp = timeStamp;  //If Rising Edge, record the start time
	else  //Otherwise its a falling edge so ..	
	{
		servoIsrData.stopTimeStamp = timeStamp;	//Record the stop time..
		servoIsrData.dataReady = true;			//And signal that we have a value
	}
}



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& SERVO ISR SUPPORT FUNCTIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/


inline void servo_begin(void)
{
	servoIsrData.dataReady = false;
	
	//Configure PCINT0 PIN (PB0) as an input
	DDRB &= ~_BV(DDB0);
	
	//DISABLE PULLUP RESISTOR (PB0)
	PORTB &= ~_BV(PORTB0);
	
	//The low level of INT0 generates an interrupt request
	MCUCR &=	~_BV(ISC01);
	MCUCR |= _BV(ISC00);
	
	//Enable External Interrupt INT0
	GICR |= _BV(INT0); 
	
	
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
	
	
	if (servo_changed()) 
	{
		currentServo = servo_value_uS();
		if (currentServo != lastServo)
		{
			currentSpeed = abs(currentServo - 1500)*2;
			lastServo = currentServo;
			gimbal.set_speed_rpm(currentSpeed);		
		}
	}
						
	gimbal.tickle();	
}


