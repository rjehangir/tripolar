/* Tripolar Stepper Motor Driver
-------------------------------
 
Title: Atmega8 Tripolar Stepper Motor Driver
Description: See README.md

-------------------------------
The MIT License (MIT)

Copyright (c) 2015 by Rustom Jehangir, Blue Robotics Inc.
                      Author #2, Company #2
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
&&& USER DEFINED VALUES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

#define NUM_SCRIPT_ENTRIES  6
/**< The number of script records contained in each timerScript list. */

#define TIMER_FREQ_KHZ 16000

#define COIL_RATIO_NUM 50
#define COIL_RATIO_DEN 7
	/**< The number of sine cycles each coil needs to go through for motor to 
	 * make on rotation												 */

#define FET_SWITCH_TIME_US  2
	/**< FETS do not switch on or off instantly. There will be a delay from the time that we initiate
	 * turning off a fet, to the time that the FET is truly off. Since the we running half H bridges,
	 * a condition where both the high and low side FET are turned on will create a short from power
	 * to ground. If we switched the High and Low side FETs at exactly the same time, the turn off delay
	 * would create a time window where both FETs are on at the same time. To prevent this, will implement
	 * FET_SWITCH_TIME_US delay (in microseconds) between the time that we turn one fet on an h bridge off
	 * and the time that we turn the other FET (on the same HBridge) on.								*/


#define MIN_TIMER_OCR_US 5
	/**< When we are running the PWM ISR, we will be processing timer interrupts. We will also be 
	 * setting the next timer expiration from within the ISR. If the next timer interrupt is 
	 * set too close to the current time, we could have issues where we might might miss the next
	 * interrupt while we are busy setting it up. To prevent this, we set a rule which says that 
	 * if we are within MIN_TIMER_OCR_US from the next timer expire, we will remain in the ISR
	 * to wait for the next event, rather than risk leaving the ISR.									*/
	
 #define PWM_FREQ_KHZ 2 ///< The PWM frequency in Hz
 
 #define PWM_CONTROL_FULL_SCALE_CNTS  1024
	/**< Controls what number is 100% pulse width for A,B and C found in pwmData_T */
 
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/



#define  FET_SWITCH_TIME_CNT (uint16_t)(FET_SWITCH_TIME_US*(TIMER_FREQ_KHZ/1000))  //FET_SWITCH_TIME_US converted to timer counts
#define  MIN_TIMER_OCR_CNT   (uint16_t)(MIN_TIMER_OCR_US*(TIMER_FREQ_KHZ/1000))    //MIN_TIMER_OCR_US converted to timer counts
#define PWM_CYCLE_CNT	     (uint16_t)(TIMER_FREQ_KHZ/PWM_FREQ_KHZ)			   //Number of timer counts in one PWM cycle



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STRUCTURE DEFINITIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/


		/*----------------------------------------------------------------------------------------------*/
		/** Specifies commands which the pwmISR can execute.
		 *	In the pwm timer interrupt routine, we define timer expirations, and things to do when 
		 *  that timer expires. This enumerated type is used to command what happens when that
		 *  timer expires.																				*/
		typedef enum pwmCommand_E
		{
			ePwmCommand_START,	
				/**<Start of the PWM Cycle. All channels are switch to high side to start the pulse width.
				 *  (high side  A,B,C = on, low side A,B,C = off )										*/
			ePwmCommand_OFFA,		
				/**< The PWM on period has ended for channel A has expired. Turn off the high side fet
	 			 *  so that both the high and the low side FET has are off, allowing time for the high 
				 *  side FET to turn off.
				 *  (Channel A - FET_HIGH = OFF, FET_LOW = OFF)											*/
			ePwmCommand_LOWA,
				/**< Assuming that the high side FET was successfully turned off, and the high side FET
				 * was allowed time to settle to the off position, we will now turn on the low side FET 
				 * of the h-bridge for channel A.
				 *  (Channel A - FET_HIGH = OFF, FET_LOW = ON)											*/
			ePwmCommand_OFFB, 	///< See eTimerCommand_OFFA. (Channel B - FET_HIGH = OFF, FET_LOW = OFF)
			ePwmCommand_LOWB,	    ///< See eTimerCommand_LOWA	 (Channel B - FET_HIGH = OFF, FET_LOW = ON)
			ePwmCommand_OFFC,		///< See eTimerCommand_OFFA. (Channel C - FET_HIGH = OFF, FET_LOW = OFF)
			ePwmCommand_LOWC,     ///< See eTimerCommand_LOWA	 (Channel C - FET_HIGH = OFF, FET_LOW = ON)
			ePwmCommand_ALLOFF,   
				/**< The PWM cycle has completed, We will turn off all fets on all channels. There will be 
				 *  a time delay between now, and eTimerCommand_START which will allow time for the FETs
				 *  to respond to being shutoff, before turning back on again.
				 *   (high side  A,B,C = OFF, low side A,B,C = OFF )									*/
			ePwmCommand_END_OF_ENUM 
				/**< This is used buy the software for determining if a variable of this type holds a valid 
				 * value.																				*/
		}pwmCommmand_T;
		
		
		/*----------------------------------------------------------------------------------------------*/
		/** This contains information to define a single timer expiration for the pwm ISR. An array
		 * of this structure is enough to completely desribe a pwm cycle.								*/
		typedef struct pwmEntry_S
		{
			pwmCommand_T command;  
				/**< What behavior to execute when the timer expires. See definition of pwmCommmand_T	*/
			int16_t	deltaTime; 
				/**< What value the timer should be at when this command is executed. This is referenced
				 * as a delta from the time that the previous command was executed.						*/						
		}pwmEntry_T;
		
		
		/*----------------------------------------------------------------------------------------------*/
		/**< A variable of this type holds all data used by the pwm interrupt service routine. 
		 * This is used to both hold the data. Some members of this structure are changed outside the 
		 * ISR and serve as a means of controlling the ISR's behavior.									
		 *
		 * There are 2 tables. Each table defines the PWM cycle. While one of these tables is being 
		 * executed by the ISR, the second table is free to be loaded with information for a new PWM 
		 * cycle. the isActiveTableA member is used to indicate which of these 2 tables is assigned 
		 * to the ISR. The changeTable variable is used to initiate a table change. isActiveTableA is 
		 * set by the ISR																				*/
		typedef struct pwmIsrData_S
		{
			pwmEntry_T tableA[8];		
				/**<table which defines what actions happen at what time during the PWM cycle.			*/
			pwmEntry_T tableB[8];
				/**< An alternate to table A.															*/
			bool isActiveTableA;
				/**< This indicates which table is being executed by the ISR. When set to true,
				 *	tableA is being used by the ISR, when false, tableB is. 
				 *  DEFAULT: true																		*/
			bool changeTable; 
				/**< This is set to true to instruct the ISR to change tables when starting its
				 * next PWM cycle. the ISR will set this to false once the table is changed. 
				 * While this is set to true, neither of the tables should be changed. When it is false
				 * you may change the non active table (as defined by isActiveTableA)					*/	
			pwmEntry_T *pEntry;
				/**< Pointer to the current entry in the currently active table. This is used 
				 * by the ISR to quicky access the table without having to do pointer multiplication
				 * to find it's position in the current table.			 		
				 * DEFAULT: tableA[0]																	*/
			bool enabled; 
				/**< When true, the ISR will run, when false, the ISR will return without 
				 *	doing anything.		
				 *  DEFAULT: false																		*/
		}pwmIsrData_T;
		
		typedef struct pwmChannelEntry_S
		{
			uint16_t dutyCycle; 
				/**< PWM Duty Cycle for Channel. Range is from 0 to PWM_CONTROL_FULL_SCALE_CNTS
				 * where a value of PWM_CONTROL_FULL_SCALE_CNTS indicates 100% pulse width.				*/
			bool used; 
				/**< This is used during sorting to indicate whether this entry was already already
				 * used on a previous sort so that we dont reuse the entry on the next sort.			*/			
		}pwmChannelEntry_T;
		
		/*----------------------------------------------------------------------------------------------*/
		/** Identifies array elements for PWM channel in pwmChannel array.								*/
		typedef enum pwmChannels_E
		{
			ePwmChannel_A = 0,
			ePwmChannel_B,
			ePwmChannel_C,
			ePwmChannel_COUNT					
		}pwmChannels_T;
		

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




		

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTION PROTOTYPES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	void setup(void);
	void loop(void);
	void processPWM(void);
	
	inline int16_t rpmToDelay(int16_t rpm) {return (COIL_RATIO_DEN * (uint32_t)TIMER_FREQ) / ((uint32_t)rpm * COIL_RATIO_NUM * 6);}
		
	

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
	 
	volatile pwmIsrData_T pwmIsrData;
	/**< Holds all information used by the pwm  timer interrupt service routine. 
	 * See the declaration of pwmIsrData_T for more information	 */
	
	
	pwmChannelEntry_T pwmChannel[ePwmChannel_COUNT];
	/**< Holds non ISR data related to controlling the PWM channels. Each element in the array 
	 * corresponds to a pwm channel as indexed by pwmChannels_T. The user sets each of these element
	 * to configure the pwm. He then calls setPwmIsr() which will read this data and setup pwmIsrData  */

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
		
    uint16_t incrementDelay = 16000;
		/**< The amount of time to wait before incrementing the motor rotor one step. This is 
		 * the speed control for the motor																*/


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
		if (!pwmIsrData.enabled) return;
		do {
			switch (pwmIsrData.pEntry->command)
			{
				case ePwmCommand_START:
					ApFETOn();
					BpFETOn();
					CpFETOn();
					break;
				case ePwmCommand_OFFA:
					ApFETOff();
					break;
				case ePwmCommand_LOWA:
					AnFETOn();
					break;
				case ePwmCommand_OFFB:
					BpFETOff();
					break;
				case ePwmCommand_LOWB:
					BnFETOn();
					break;
				case ePwmCommand_OFFC:
					CpFETOff();
					break;
				case ePwmCommand_LOWC:
					CnFETOn();
					break;
				case ePwmCommand_ALLOFF:
					lowSideOff();
					
					pwmIsrData.pEntry  = (pwmIsrData.isActiveTableA ? pwmIsrEntry.tableA : pwmIsrEntry.tableB);										
						/* Go to the beginning of the next table */
					pwmIsrData.changeTable = false;															
						/* In theory, the user sets changeTable to force a change in the table, in reality
						 * isActiveTableA is enough. However, the user will look at changeTable to see if 
						 * the change over was made, so that he knows when he can start writing to the 
						 * free table again. so we reset the flag here.									*/
					break;				
				default:
					//Something is very wrong, stop processing the ISR
					pwmIsrData.enabled	 = false;
					break;					
			}
			
			//If we are on the last command on the PWM sequence, go to beginning of the next table.
			if (pwmIsrData.pEntry->command != ePwmCommand_ALLOFF) pwmIsrData.pEntry++;
						
			OCR1A +=  pwmIsrData.pEntry->deltaTime;  //Configure the time of the next interrupt.
			if (OCR1A - TCNT1 > MIN_TIMER_OCR_CNT) break; //If the expiration time is not too close, then exit ISR		
				
		} while (OCR1A - TCNT1 < 0)	//Otherwise, stay in ISR and wait for the next timer expiration.	
		
			
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
		
		TCCR2 &= ~_BV(CS20); // make sure CS20 and CS22 are NOT SET
		TCCR2 |= _BV(CS21); // set CS21 for prescaler of 8
		TCCR2 &= ~_BV(CS22);		
	
		// Reset timer
		TCCR1A = 0;
		TCCR1B = 0;
		TIMSK = 0;
		TCNT1 = 0;
		// Set compare match register to overflow immediately
		OCR1A = 65535;
		// Turn on CTC mode for timer 1
	//	TCCR1B |= _BV(WGM12);
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
		
		incrementDelay = rpmToDelay(1);  //Set the motor to 250 RPM
		
		incrementRotor();

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
		static int8_t rampValue = 1;
		static int16_t lastTimer = 0;
		static int16_t speed = 1;
		
		static uint16_t milliSeconds = 0;
		static uint16_t lastMilliTime = 0;
		const  uint16_t milliExpire = 16000;
		
		static uint16_t lastIncrementTime = 0;
		
		
		static uint16_t lastSpeedChangeTime_ms = 0;
		static uint16_t speedChangeDelay_ms = 50; 
		
		static bool rotorChanged = true;
			/**< When true, indicates that the rotor position was changed and that we need to update the PWM values. */
			

		
		
		
		
		//Update the Milliseconds Timer 
		if (TCNT1 - lastMilliTime > milliExpire)
		{
			 milliSeconds++;
			 lastMilliTime = TCNT1;
			
		}
		
		if (TCNT1 - lastIncrementTime > incrementDelay)
		{
			lastIncrementTime = TCNT1;
			incrementRotor();			
		}
		
		
		
		if (milliSeconds - lastSpeedChangeTime_ms > speedChangeDelay_ms)
		{		
			lastSpeedChangeTime_ms = milliSeconds;				
			incrementDelay = rpmToDelay(speed++);				
		} 

		//Update the PWM, only if it has completed a cycle and is ready for a new value.
		if (!state.newValue)
		{						
			state.pulseBufferA = ((1*(pwmSin[currentStepA]-128))/1) +128;
			state.pulseBufferB = ((1*(pwmSin[currentStepB]-128))/1) +128;
			state.pulseBufferC = ((1*(pwmSin[currentStepC]-128))/1) +128;
			state.newValue = true;			
			rotorChanged = false;						
		}
		
		processPWM();

		//delay(0);
	}
	
	
	
	void incrementRotor(void)
	{
		
		if ( forward ) increment = -1;
		else increment = 1;

		currentStepA += increment;
		currentStepB += increment;
		currentStepC += increment;

		if (currentStepA>sineArraySize) currentStepA = 0;
		if (currentStepA<0) currentStepA = sineArraySize;

		if (currentStepB>sineArraySize) currentStepB = 0;
		if (currentStepB<0) currentStepB = sineArraySize;

		if (currentStepC>sineArraySize) currentStepC = 0;
		if (currentStepC<0) currentStepC = sineArraySize;				
	};
	
	void processPWM(void)
	{
		if ( TCNT2 > 255 - OFFDELAY ) {
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
		if ( TCNT2 > state.pulseWidthA+OFFDELAY ) AnFETOn();
		// Turn high side on until duty cycle expires
		if ( TCNT2 < state.pulseWidthA ) { ApFETOn(); AnFETOff(); }
		else ApFETOff();

		// Turn low side back on after delay
		if ( TCNT2 > state.pulseWidthB+OFFDELAY ) BnFETOn();
		// Turn high side on until duty cycle expires
		if ( TCNT2 < state.pulseWidthB ) { BpFETOn(); BnFETOff(); }
		else BpFETOff();

		// Turn low side back on after delay
		if ( TCNT2 > state.pulseWidthC+OFFDELAY ) CnFETOn();
		// Turn high side on until duty cycle expires
		if ( TCNT2 < state.pulseWidthC ) { CpFETOn(); CnFETOff(); }
		else CpFETOff();
		
	}
	
	
	/********************************************************************************/
	/* FUNCTION: setPwmIsr()
	*  DESCRIPTION:	
	/**		This reads pwmCommand and updates pwmIsrData. When setting the PWM 
	 *      isr The procedure should be to set the values in pwmCommand and then
	 *      call this function. */
	/********************************************************************************/
	void setPwmIsr(void)
	{
		pwmEntry_T *p;
		pwmChannelEntry_T *c;
		uint8_t nextPwmIndex;
		uint16_t minValue = 65535;
		uint16_t deltaTime;
		uint16_t endTime;
		uint16_t nextStartTime;
		pwmCommmand_T nextCommand;
		uint8_t command;
		
		
		
		//---------------------------------------------------------------------------------
		//FIRST PWM TABLE ENTRY - START COMMAND
		//---------------------------------------------------------------------------------
		p  = (pwmIsrData.isActiveTableA ? pwmIsrEntry.tableB : pwmIsrEntry.tableA);	
		
		//Setup First Entry State ePwmCommand_START. (Note this will never change)
		p->command = ePwmCommand_START;
		p->deltaTime = FET_SWITCH_TIME_CNT;
		
		
		//---------------------------------------------------------------------------------
		//SIX MORE TABLE ENTRIES - TURNING OFF EACH FET AT END OF ITS PWM DUTY CYCLE
		//---------------------------------------------------------------------------------
					
			
	    /*Reset used flags in the pwmChannel[] array if they were set on a previous run.
		Using separate instructions is less space than a for loop.		*/
	    pwmChannel[0].used = false;
	    pwmChannel[1].used = false;
	    pwmChannel[2].used = false;
			
		
		//There are 3 entry pairs (turn off high side + turn on low side) 
		//Create entries for each pair ... so six entries.	
		for (uint8_t entryPair = 0; entryPair<3;entryPair++)
		{			
					minValue = 65535; //Reset minValue.
					c = pwmChannel; //Reset c pointer to first entry in pwmChannel array;
					
					//Find "nextPwmIndex" which is the channel with the lowest pulse width not used yet
					for (uint8_t n = 0;n<ePwmChannel_COUNT;n++)	{						
						if (c->dutyCycle < minValue && !c->used) {
							nextPwmIndex = n;
							minValue = p->dutyCycle;
						}
						c++;
					}
										
					//Find the command to turn off that channel's high side FET 
					switch(nextPwmIndex)
					{
						case ePwmChannel_A:
							command = ePwmCommand_OFFA;
							break;
						case ePwmChannel_B:
							command = ePwmCommand_OFFB;
							break;
						case ePwmChannel_C:
							command = ePwmCommand_OFFC;
							break;
						default:
					}
					
					//Make timer ISR entry for that command
					p->command = command;
					p->deltaTime  =  ((uint32_t) c->dutyCycle * PWM_CYCLE_CNT) / PWM_CONTROL_FULL_SCALE_CNTS;
					
					//Make another timer ISR entry for turning on low side for that channel.
					p++;
					p->command = command + 1; 
						/* IN enum, the command for turning on low side is always immediately after
						 * the one for turning off the high side so we can +1 rather than using another
						 * case statement.*/
					p->deltaTime = FET_SWITCH_TIME_CNT;
					c->used = true;
		}
		
		
		//---------------------------------------------------------------------------------
		// LAST PWM TABLE ENTRY - TURN ALL FETS OFF
		//---------------------------------------------------------------------------------
		p++;
		p->command = ePwmCommand_ALLOFF;
		p->deltaTime = FET_SWITCH_TIME_CNT;
		
		
		
	}
	
	

