/***************************************************************************************//**
 * @brief C implementation file for bldcPwm class.	
 * @details
 *		The documentation strategy is to document the header file as much as possible
 *		and only comment this CPP file for things not already documented in the header
 *	    file.  
 *		
 *		See bldcPwm.h for an in-depth description of this class, its methods,
 *		and its properties.
 * @author Alan Nise
 * @
 *//***************************************************************************************/


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INCLUDES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "afro_nfet.h"
#include "fets.h"
#include <avr/io.h>
#include "bldcPwm.h"

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/


#define  FET_SWITCH_TIME_CNT (uint16_t)(bldcPwm::kFetSwitchTime_uS*(bldcPwm::kTimerFreq_Khz/1000)) 
	 /**< kFetSwitchTime_uS converted to timer counts */
#define  MIN_TIMER_OCR_CNT   (uint16_t)(bldcPwm::kMinTimerDelta_uS*(bldcPwm::kTimerFreq_Khz/1000)) 
	 /**< kMinTimerDelta_uS converted to timer counts  */
#define  PWM_CYCLE_CNT	     (uint16_t)(bldcPwm::kTimerFreq_Khz/bldcPwm::kPwmFreq_Khz)	
     /**<Number of timer counts in one PWM cycle */




/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STRUCTURES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

		/************************************************************************************************/
		/* ENUM: pwmCommand_E																			*/
		/** Specifies commands which the pwmISR can execute.
		 *	In the pwm timer interrupt routine, we define timer expirations, and things to do when 
		 *  that timer expires. This enumerated type is used to command what happens when that
		 *  timer expires.			
		 *  RULE #1 IS: Nobody talks about fight club... just kidding. 
		 *  RULE #1 IS: The ePwmCommand_OFFx commands must be immediately after their ePwmCommand_OFFx
		 *			    counterpart for the same channel.	See update method.							*/
		/************************************************************************************************/		
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
			}pwmCommand_T;			
			
			

		/************************************************************************************************/
		/* STRUCT: pwmEntry_S																			*/
		/** This contains information to define a single timer expiration for the pwm ISR. An array
	     * of this structure is enough to completely describe a pwm cycle.								*/
		/************************************************************************************************/
			typedef volatile struct pwmEntry_S
			{
				volatile pwmCommand_T command;  
					/**< What behavior to execute when the timer expires. See definition of pwmCommmand_T	*/
				volatile int16_t	deltaTime; 
					/**< What value the timer should be at when this command is executed. This is referenced
					 * as a delta from the time that the previous command was executed.						*/						
			}pwmEntry_T;
		
		
		
		
		/************************************************************************************************/
		/* STRUCT: pwmIsrData_S																			*/	
		/**< A variable of this type holds all data used by the pwm interrupt service routine. 
		 * This is used to both hold the data. Some members of this structure are changed outside the 
		 * ISR and serve as a means of controlling the ISR's behavior.									
		 *
		 * There are 2 tables. Each table defines the PWM cycle. While one of these tables is being 
		 * executed by the ISR, the second table is free to be loaded with information for a new PWM 
		 * cycle. the isActiveTableA member is used to indicate which of these 2 tables is assigned 
		 * to the ISR. The changeTable variable is used to initiate a table change. isActiveTableA is 
		 * set by the ISR			 		 															*/
		/************************************************************************************************/
			typedef volatile struct pwmIsrData_S
			{
				volatile pwmEntry_T tableA[8];		
					/**<table which defines what actions happen at what time during the PWM cycle.			*/
				volatile pwmEntry_T tableB[8];
					/**< An alternate to table A.															*/
				volatile bool isActiveTableA;
					/**< This indicates which table is being executed by the ISR. When set to true,
					 *	tableA is being used by the ISR, when false, tableB is. 
					 *  DEFAULT: true																		*/
				volatile bool changeTable; 
					/**< This is set to true to instruct the ISR to change tables when starting its
					 * next PWM cycle. the ISR will set this to false once the table is changed. 
					 * While this is set to true, neither of the tables should be changed. When it is false
					 * you may change the non active table (as defined by isActiveTableA)					*/	
				volatile pwmEntry_T *pEntry;
					/**< Pointer to the current entry in the currently active table. This is used 
					 * by the ISR to quicky access the table without having to do pointer multiplication
					 * to find it's position in the current table.			 		
					 * DEFAULT: tableA[0]																	*/
				volatile bool enabled; 
					/**< When true, the ISR will run, when false, the ISR will return without 
					 *	doing anything.		
					 *  DEFAULT: false																		*/
			}pwmIsrData_T;

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STATIC VARIABLES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	volatile  pwmIsrData_T pwmIsrData;
		/**< Holds all information used by the pwm  timer interrupt service routine. 
		 * See the declaration of pwmIsrData_T for more information	 */

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INTERRUPT SERVICE ROUTINES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	/****************************************************************************************************/
	/** @brief 
	 *		Timer1 PWM Interrupt Service Routine.
	 * @description
	 *      This ISR processes the Timer1 output compare interrupt to implement 3 PWM output channels
	 *		which are geared towards controlling a brushless DC motor. At its heart, the pwmIsrData
	 *      variable contains a structure which includes 2 table arrays, each of which separately 
	 *		implement a script, where each entry in the array specifies a time and an action. This ISR
	 *		steps through either this table, or its alternate, in order to implement the PWM.  The idea 
	 *      is to do a lot of the upfront work using the bldcPwm::update method outside the ISR in 
	 *		order to minimize the complexity of ISR. 
	 *   
	 *		This ISR allows timer1 to freerun through it's whole range, so all the output compare
	 *      timer expirations need to be relative. As such the timer expirations are specified as
	 *		deltas, and the timer output compare register value for the next expiration is dynamically
	 *      calculated and set from within this ISR.
	 *
	 *		The timer expiration is variable, and could be less that a microsecond. As such, there is a
	 *		risk that the timer could run past its expiration before we exit the ISR. To handle this, 
	 *      we specifiy a minimum time delay. If we are below this minimum, we will not exit the ISR,
	 *      rather, we will wait for the expiration from within the ISR.  This minimum value is 
	 *      set via the kMinTimerDelta_uS constant. 
	 *  
	 *      Reference pwmIsrData_T and pwmEntry_T for more details on how this ISR works.
	 *								*/
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
					
					pwmIsrData.pEntry  = (pwmIsrData.isActiveTableA ? pwmIsrData.tableA : pwmIsrData.tableB);										
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
			while (OCR1A - TCNT1 < 0);	//Otherwise, stay in ISR and wait for the next timer expiration.		
				
		} while(1);
		
		//   <========== !!!!!!! CLEAR THE INTERRUPT SOURCE !!!!!!!!
			
	}


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& CLASS IMPLEMENTATION FUNCTIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/



	 	 
	/****************************************************************************
	*  Class: bldcPwm
	*  Method: bldcPwm
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	bldcPwm::bldcPwm(void)
	{
		
	
	}


	 	 
	/****************************************************************************
	*  Class: bldcPwm
	*  Method: begin
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	void bldcPwm::begin(void)
	{
		cli();
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
		TIMSK |= _BV(OCIE1A);
		// Clear any pending interrupts
		TIFR |= _BV(OCF1A);
		sei();			
	}


	 	 
	/****************************************************************************
	*  Class: bldcPwm
	*  Method: update
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	void bldcPwm::update(void)
	{
		volatile pwmEntry_T *pIsrScriptEntry;   
			/**< Pointer to the an entry in pwm script table which we are creating.
			 * We use this point to navigate the table as we populate it. */
		pwmChannelEntry_T *pPwmChannel;
			/**< Pointer to the pwmChannel array which contains the PWM duty cycle values
			 * which we are reading in order to construct the pwm ISR script entries. */
		uint8_t currentPwmIndex; 
			/**Holds the index to the pwmChannel array, for the entry which we are currently
			 * adding to the script. These are found one by one, by sorting them from 
			 * shortest duty cycle to longest.		*/
		uint8_t command; 
			/**< Buffer the value from the switch so we can set set it after - 
			 * doing the more setting through pointer only once */
		
		//---------------------------------------------------------------------------------
		//FIRST PWM TABLE ENTRY - START COMMAND
		//---------------------------------------------------------------------------------
		pIsrScriptEntry  = (pwmIsrData.isActiveTableA ? pwmIsrData.tableB : pwmIsrData.tableA);	
		
		//Setup First Entry State ePwmCommand_START. (Note this will never change)
		pIsrScriptEntry->command = ePwmCommand_START;
		pIsrScriptEntry->deltaTime = FET_SWITCH_TIME_CNT;
				
		//---------------------------------------------------------------------------------
		//SIX MORE TABLE ENTRIES - TURNING OFF EACH FET AT END OF ITS PWM DUTY CYCLE
		//---------------------------------------------------------------------------------
					
			
	    /*Reset used flags in the pwmChannel[] array if they were set on a previous run.
		Using separate instructions is less space than a for loop.		*/
	    _pwmChannel[0].used = false;
	    _pwmChannel[1].used = false;
	    _pwmChannel[2].used = false;
			
		
		//There are 3 entry pairs (turn off high side + turn on low side) 
		//Create entries for each pair ... so six entries.	
		for (uint8_t entryPair = 0; entryPair<3;entryPair++)
		{	
																
					uint16_t shortestDutyCycle = 65535;
						/**< This variable is used while we attempt to sort the pwmChannel entries. 
						 * it holds the shortest duty cycle it has found so far as we search the pwmArray. */
		
					pPwmChannel = _pwmChannel; //Reset c pointer to first entry in pwmChannel array;
					
					//Find "nextPwmIndex" which is the channel with the lowest pulse width not used yet
					for (uint8_t n = 0; n < ePwmChannel_COUNT;n++)	{						
						if (pPwmChannel->dutyCycle < shortestDutyCycle && !pPwmChannel->used) {
							currentPwmIndex = n;
							shortestDutyCycle = pPwmChannel->dutyCycle;
						}
						pPwmChannel++; //Step to next pwmChannel
					}
					
					pPwmChannel->used = true; 
						/* Indicate we already used this pwmChannel index so that we ignore it
						 * when we search for the next entry. */
					
					//Find the command to turn off that channel's high side FET 
					// based on what index (each index is always assigned to only
					// one channel so we can check the index and know what channel,
					// and therefore what the proper command is. 
					switch(currentPwmIndex)
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
							break;
					}
					
					//---------------------------------------------------------------------------------
					// HIGH SIDE FET OFF ENTRY - TURNING OFF HIGH SIDE FET FOR THIS PWM CHANNEL
					//---------------------------------------------------------------------------------					
					pIsrScriptEntry->command = (pwmCommand_T) command;
					pIsrScriptEntry->deltaTime  =  ((uint32_t) pPwmChannel->dutyCycle * PWM_CYCLE_CNT) / kDutyCycleFullScale;
										
					//---------------------------------------------------------------------------------
					// LOW SIDE FET ON ENTRY - TURN ON LOW SIDE FET FOR THIS CHANNEL
					//---------------------------------------------------------------------------------
					pIsrScriptEntry++; //Step to next  ISR script entry.
					pIsrScriptEntry->command = (pwmCommand_T)(command + 1); 
						/* IN enum, the command for turning on low side is always immediately after
						 * the one for turning off the high side so we can +1 rather than using another
						 * case statement.*/
					pIsrScriptEntry->deltaTime = FET_SWITCH_TIME_CNT;
					
					
		} //end -  for(entryPair)
		
		
		//---------------------------------------------------------------------------------
		// LAST PWM TABLE ENTRY - TURN ALL FETS OFF
		//---------------------------------------------------------------------------------
		pIsrScriptEntry++; //Step to next  ISR script entry.
		pIsrScriptEntry->command = ePwmCommand_ALLOFF;
		pIsrScriptEntry->deltaTime = FET_SWITCH_TIME_CNT;
		
			
		//---------------------------------------------------------------------------------
		// NOW TELL THE ISR TO SWITCH TO THE TABLE WE JUST CREATED
		//---------------------------------------------------------------------------------
		cli(); //Turn off interrupts while we write to active part of ISR's data
		{
			pwmIsrData.changeTable = true;
			pwmIsrData.isActiveTableA = !pwmIsrData.isActiveTableA;			
		}		
		sei(); //Turn back up interrupts
	}


