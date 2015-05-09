/* Tripolar Stepper Motor Driver
-------------------------------
 
Title: Atmega8 Tripolar Stepper Motor Driver
Description: See README.md

-------------------------------
The MIT License (MIT)

Copyright (c) 2015 by Rustom Jehangir, Blue Robotics Inc.
                      Alan Nise, OceanLab, LLC.
                      Author #3, Company #3

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-------------------------------*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "afro_nfet.h"
#include "fets.h"
#include <avr/io.h>

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& DEFINED VALUES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

#define NUM_SCRIPT_ENTRIES  6
/**< The number of script records contained in each timerScript list. */



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STRUCTURE DEFINITIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

		/*---------------------------------------------------------------------------------------------------*/
		/** @typedef Structure to hold timerISR configuration and state data. */
		typedef struct state_S
		{
			bool on;					///< When true, indicates that the PWM output is turned on. Currently not used.
			bool state;					///<Currently  Not Used.
			uint8_t pulseWidthA;		///< PWM Pulse Width for First Motor Phase. Ranges from 0 to 255, where 255 is 100%
			uint8_t pulseWidthB;		///< See pulseWidthA
			uint8_t pulseWidthC;		///< See pulseWidthA
			
			uint8_t pulseBufferA;		///< Buffered version of pulseWidthA with value to copied to pulseWidthA at begining of PWM cycle
			uint8_t pulseBufferB;		///< See pulseBufferA
			uint8_t pulseBufferC;		///< See pulseBufferA
			
			bool newValue;				
				/**< When true, new value has been written to pulseBufferX. Tells ISR to copy new value and Main loop not to calculate
				 * new PWM */
			
			
		} state_T;



		
		/*-----------------------------------------------------------------------------------------------------*/
		/** @brief
		 *		This structure is used to create a variable which holds all data for the timer ISR. 
		 *  @description 
		 *      The idea is that we provide the timer with a linked list, with each record in the list
		 *	telling the timer ISR what needs to be done at that 
		 *  specific time. 
		 * 
		 *  This structure was designed to minimize the number of instructions which need to be executed 
		 *  within the ISR. The idea is to make ISR insanely simple, at the expense of significantly
		 *  more work which needs to be performed by the main loop to prepare this structure. 
		 *
		 *  Fundamentally, we will be using the timer ISR to generate a PWM signal, so 
		 *  we will be needing to turn on and off i/o pins. Rather than wasting time writing to these
		 *  pin individually, we will write to the pins all at once using the portAndMask and portOrMask
		 *  portion of this structure. 
		 *
		 *  These records will need to be sorted in order of ascending timer counts. To assist with this 
		 *  we will be making a linked list, where each record points to the next one. This was implemented
		 *  in the nextRecord member which is a 16 bit pointer, rather than an array index. While the array
		 *  index takes less space, the compiler will have to execute its own pointer math to map the array
		 *  index to the next records memory location. */
		typedef struct timerScriptEntry_S
		{
			int16_t timerCount;			//What the timer count should be when this record is executed.
			uint8_t portAndMask;		//The i/o port which controls the pins will be ANDed by this mask.
			uint8_t portOrMask;			//The i/o port which controls the pins will be ORed by this mask.
			void *nextRecord;  
				/**<Pointer to the next record in the linked list. A NULL indicates that this is the last
				 * record of the linked list.																	*/	
		}timerScriptEntry_T;




		/*-----------------------------------------------------------------------------------------------------*/
		/** @typdef This structure is used to hold all the information used by the timer ISR. Note that within 
		 *	this structure we have 2 entirely separate scripts. This is done in order to allow one script
		 *  to continue running while the other one is being populated within the main loop. We also 
		 *  do this to allow for synchronization to occur, so that when a new PWM value is written, the 
		 *  PWM cycle for the previous value is allowed to complete, and the script for the new PWM value
		 *  is loaded just before the next cycle. Hopefully this will lead to glitch free transitions to 
		 *  new values.																							*/
		typedef struct timerISR_S		  																		
		{
			timerScriptEntry_T *pCurrentScript;
					/**< Pointer to the first record of the timerScript which is currently being run			*/
			timerScriptEntry_T *pNextScript;
					/**< Pointer to the first record of the timerScript which must be run when the current
					 * script completes execution.																*/
			timerScriptEntry_T scriptList[2][NUM_SCRIPT_ENTRIES];
				   /**<Multidimensional array holding 2 scripts, and the entries for those lists.	The first
					* script is accessed by .scriptList[0,n] and the second by .scriptList[1,n]					*/			
		}timerISR_T;
		
		

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTION PROTOTYPES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	void setup(void);
	void loop(void);
	void processPWM(void);

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& CONSTANT DECLARATIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/




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
	const int pwmSin[] = 
	
	{
			128, 132, 136, 140, 143, 147, 151, 155, 159, 162,   //0 to 9
			166, 170, 174, 178, 181, 185, 189, 192, 196, 200,   //10 to 19
			203, 207, 211, 214, 218, 221, 225, 228, 232, 235,   //20 to 29
			238, 239, 240, 241, 242, 243, 244, 245, 246, 247,	//30 to 39
			248, 248, 249, 250, 250, 251, 252, 252, 253, 253,	//40 to 49
			253, 254, 254, 254, 255, 255, 255, 255, 255, 255,	//50 to 59
			255, 255, 255, 255, 255, 255, 255, 254, 254, 254,	//60 to 69
			253, 253, 253, 252, 252, 251, 250, 250, 249, 248,	//70 to 79
			248, 247, 246, 245, 244, 243, 242, 241, 240, 239,	//80 to 89
			238, 239, 240, 241, 242, 243, 244, 245, 246, 247,	//90 to 99
			248, 248, 249, 250, 250, 251, 252, 252, 253, 253,   //100 to 109
			253, 254, 254, 254, 255, 255, 255, 255, 255, 255,   //110 to 119
			255, 255, 255, 255, 255, 255, 255, 254, 254, 254,   //120 to 129
			253, 253, 253, 252, 252, 251, 250, 250, 249, 248,   //130 to 139
			248, 247, 246, 245, 244, 243, 242, 241, 240, 239,   //140 to 149
			238, 235, 232, 228, 225, 221, 218, 214, 211, 207,   //150 to 159
			203, 200, 196, 192, 189, 185, 181, 178, 174, 170,   //160 to 169
			166, 162, 159, 155, 151, 147, 143, 140, 136, 132,   //170 to 179
			128, 124, 120, 116, 113, 109, 105, 101,  97,  94,   //180 to 189
			90,  86,  82,  78,  75,  71,  67,  64,  60,  56,    //190 to 199
			53,  49,  45,  42,  38,  35,  31,  28,  24,  21,    //200 to 209
			18,  17,  16,  15,  14,  13,  12,  11,  10,   9,    //210 to 219
		  	 8,   8,   7,   6,   6,   5,   4,   4,   3,   3,    //220 to 229
			 3,   2,   2,   2,   1,   1,   1,   1,   1,   1,    //230 to 239
			 1,   1,   1,   1,   1,   1,   1,   2,   2,   2,    //240 to 249
			 3,   3,   3,   4,   4,   5,   6,   6,   7,   8,    //250 to 259
			 8,   9,  10,  11,  12,  13,  14,  15,  16,  17,    //260 to 269
			 18,  17,  16,  15,  14,  13,  12,  11,  10,   9,   //270 to 279
			 8,   8,   7,   6,   6,   5,   4,   4,   3,   3,    //280 to 289
			 3,   2,   2,   2,   1,   1,   1,   1,   1,   1,    //290 to 299
			 1,	  1,   1,   1,   1,   1,   1,   2,   2,   2,    //300 to 309
			 3,   3,   3,   4,   4,   5,   6,   6,   7,   8,    //310 to 319
			 8,   9,  10,  11,  12,  13,  14,  15,  16,  17,    //320 to 329
			18,  21,  24,  28,  31,  35,  38,  42,  45,  49,    //330 to 339
			53,  56,  60,  64,  67,  71,  75,  78,  82,  86,    //340 to 349
			90,  94,  97, 101, 105, 109, 113, 116, 120, 124     //350 to 359
	};

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& VARIABLE DECLARATIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	volatile state_T state = {0,0,0,0,0};	
	/**< Holds all information used by the timer interrupt service routine. 
	 * See the declaration of state_T for more information												*/

	int16_t currentStepA;			
		/**< Holds an index to the current phase for motor coil. The motor is moved by incrementing this 
		 * variable, along with equivalent variables for the other 2 motor coils. Note that each of the 
		 * three currentStep variable have a precise difference in phases.  */
	int16_t currentStepB;	//See currentStepA for more information.
	int16_t currentStepC;	//See currentStepA for more information.
	
	
	int16_t bufferStepA;	
		/**< Same as currentStepA, but a buffered version we use to hold the value pending completion of
		 * a PWM cycle at which time we will write the value; */
	int16_t bufferStepB;	///< See bufferStepA
	int16_t bufferStepC;	///< See bufferStepA
	
	bool newValueReady; 
		/**< When true. indicates that bufferStepX has a new value. Process PWM will load this into 
			 currentStepX at the beginning of the PWM cycle. Also, the calculation of the next step 
			 will be suppressed while this is set to true */
	
	
	int16_t sineArraySize;	
		/**< This is the size of the pwmSine table and serves to mark the limit for the currentStep
		 * variables. When they read this value, they will loop around back to 0.						*/
	int8_t increment; 
		/**< The value to increment the currentStep variable for each period. Currently this is 
		 * only +1 and -1 and is used to control direction. In later times, this may be increased
		 * to get faster rotation by skipping step positions.											*/
	bool forward; 
		/**< Direction control for the motor. If true, the motor goes forward, if false
		 * the motor goes in reverse.																	*/


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INTERRUPT SERVICE ROUTINES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	
	

	/****************************************************************************************************/
	/** @brief 
	 *		Timer1 Interrupt Service Routine.
	 * @description
	 *		This function is called by the interrupt processing system whenever 
	 *      the timer1 reaches its specified value.														*/
	/****************************************************************************************************/			
	ISR(TIMER1_COMPA_vect) 
	{
		//processPWM();
	}
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTION IMPLEMENTATION
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	



	/********************************************************************************/
	/** @brief 
	 *		 C Main Function.
	 *  @description
	 *		This function is automatically executed when the uC starts up			*/
	/********************************************************************************/	
	int main(void)
	{
		setup();
		while(1)
		{
			//TODO:: Please write your application code 
			loop();
		}
	}




	/********************************************************************************/
	/** @brief 
	 *		 System Setup Function
     *  @description
	 *		This function is modeled after the Arduino setup function. This processes
	 *      all of the setup steps required at startup. We are keeping this function
	 *		to allow a return to Ardunio sketch compatibility at a later date.
	 *																				*/
	/********************************************************************************/	
	void setup() {
		cli();

		boardInit();

		// Set up timer two, which we will watch with Timer1 interrupt 
		//  001 = no prescaling.
		//  010 = divide by 8
		
		TCCR2 &= ~_BV(CS21);				
		TCCR2 |= _BV(CS22);
		TCCR2 &= ~_BV(CS21);
	
		// Reset timer
		TCCR1A = 0;
		TCCR1B = 0;
		TIMSK = 0;
		TCNT1 = 0;
		// Set compare match register to overflow immediately
		OCR1A = 0;
		// Turn on CTC mode for timer 1
		TCCR1B |= _BV(WGM12);
		// Set for no prescaler
		TCCR1B |= _BV(CS10);
		// Enable timer compare interrupt
	//	TIMSK |= _BV(OCIE1A);
		// Clear any pending interrupts
   		TIFR |= _BV(OCF1A);
		
		

	
		sei();

		sineArraySize = sizeof(pwmSin)/sizeof(int);
		int phaseShift = sineArraySize/3;
		currentStepA = 0;
		currentStepB = currentStepA + phaseShift;
		currentStepC = currentStepB + phaseShift;

		sineArraySize--;

		//delay(100);
		//for (int n=0;n<1000;n++)asm("nop");
	}

uint8_t phase;




	/********************************************************************************/
	/** @brief 
	 *		 System Loop Function
	 *  @description
	 *		This function is modeled after the Arduino loop function. This function
	 *		is called continuously and is where any tasks which need to be repeated
	 *      should be placed. 
	 *																				*/
	/********************************************************************************/	
	void loop() {
		static int8_t loopCount = 0;
		static int32_t rampCount = 10000;
		static int8_t rampValue = 0;
		
		processPWM();
		
		if (!state.newValue)
		{
			loopCount = 0;
			if (rampCount++ >=10000){
					if (rampValue >= 0) rampValue++;
					rampCount = 0;
			}
			state.pulseBufferA = ((1*(pwmSin[currentStepA]-128))/1) +128;
			state.pulseBufferB = ((1*(pwmSin[currentStepB]-128))/1) +128;
			state.pulseBufferC = ((1*(pwmSin[currentStepC]-128))/1) +128;
			state.newValue = true;
						
			if ( forward ) increment = rampValue;
			else increment = -1*rampValue;

			currentStepA += increment;
			currentStepB += increment;
			currentStepC += increment;

			if (currentStepA>sineArraySize) currentStepA = 0;
			if (currentStepA<0) currentStepA = sineArraySize;

			if (currentStepB>sineArraySize) currentStepB = 0;
			if (currentStepB<0) currentStepB = sineArraySize;

			if (currentStepC>sineArraySize) currentStepC = 0;
			if (currentStepC<0) currentStepC = sineArraySize;
		}

		//delay(0);
	}
	
	
	
	void processPWM(void)
	{
		if ( TCNT2 > 253 ) {
			// Turn low side back off before turning high side on			
			lowSideOff();
			
			if (state.newValue) {
			state.pulseWidthA = state.pulseBufferA;
			state.pulseWidthB = state.pulseBufferB;			
			state.pulseWidthC = state.pulseBufferC;
			state.newValue = false;
			}			
		}

		// Turn low side back on after delay
		if ( TCNT2 > state.pulseWidthA+10 ) AnFETOn();
		// Turn high side on until duty cycle expires
		if ( TCNT2 < state.pulseWidthA ) { ApFETOn(); AnFETOff(); }
		else ApFETOff();

		// Turn low side back on after delay
		if ( TCNT2 > state.pulseWidthB+10 ) BnFETOn();
		// Turn high side on until duty cycle expires
		if ( TCNT2 < state.pulseWidthB ) { BpFETOn(); BnFETOff(); }
		else BpFETOff();

		// Turn low side back on after delay
		if ( TCNT2 > state.pulseWidthC+10 ) CnFETOn();
		// Turn high side on until duty cycle expires
		if ( TCNT2 < state.pulseWidthC ) { CpFETOn(); CnFETOff(); }
		else CpFETOff();
		
	}

