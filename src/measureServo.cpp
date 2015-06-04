/***************************************************************************************//**
 * @brief C implementation file for measureServo class.	
 * @details
 *		The documentation strategy is to document the header file as much as possible
 *		and only comment this CPP file for things not already documented in the header
 *	    file.  
 *		
 *		See measureServo.h for an in-depth description of this class, its methods,
 *		and its properties.
 * @author Alan Nise, Oceanlab LLC
 * @
 *//***************************************************************************************/




/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INCLUDES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

#include "measureServo.h"
#include "millis.h"
#include <stdlib.h>
#include <avr/interrupt.h>

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	#define T1ICP_IRQ_OFF() TIMSK &= ~_BV(TICIE1); //Turn off input compare interrupt while accessing its data
	#define T1ICP_IRQ_ON() TIMSK |= _BV(TICIE1);
	

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
		volatile uint16_t startTimeStamp;  ///<Time stamp of the pulse begining in uS.
		volatile uint16_t stopTimeStamp;   ///<Time stamp of the pulse end in uS.
		bool dataReady;	
			/**< Set True by the ISR when a pulse has been detected and measured. The ISR will suspend 
			 * any other measurements until this is set false by the application. */
		bool waitRising; //If true, we are waiting for a rising edge, otherwise a falling	
	}servoIsrData_T;



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& VARIABLES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	static servoIsrData_T servoIsrData; //Variable used to store data used to interact with the ISR.

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INTERRUPT SERVICE ROUTINES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	/****************************************************************************	
	*  ISR: TIMER1_CAPT_vect
	*	Description:
	*		Triggered upon a change on the ICP1 pin. This ISR us used to 
	*		measure the pulse width WITHOUT using the Timer1 Input Capture 
	*		register.
	****************************************************************************/	
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
&&& CLASS METHOD IMPLEMENTATION FUNCTIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	 	 
	/****************************************************************************
	*  Class: measureServo
	*  Method: measureServo
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/
	measureServo::measureServo(void)
	{
		asm("NOP");
	}
		
	/****************************************************************************
	*  Class: measureServo
	*  Method: begin
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/				
	void measureServo::begin(void)
	{		
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
		
	/****************************************************************************
	*  Class: measureServo
	*  Method: measureServo
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/			
	bool measureServo::changeDetected(void)
	{
		T1ICP_IRQ_OFF(); //Turn off input compare interrupt while accessing its data
			bool retVal = servoIsrData.dataReady;
		T1ICP_IRQ_ON(); //Turn it back on		
		return retVal;
	}
		
	/****************************************************************************
	*  Class: measureServo
	*  Method: measureServo
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	uint16_t measureServo::value_uS(void)
	{
		volatile uint16_t stopTime, startTime;
		T1ICP_IRQ_OFF(); //Turn off input compare interrupt while accessing its data
			stopTime = servoIsrData.stopTimeStamp;
			startTime = servoIsrData.startTimeStamp;			
			servoIsrData.dataReady = false;
		T1ICP_IRQ_ON(); //Turn it back on								
		return stopTime - startTime;
	}
		 