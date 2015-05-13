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

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/


#define  FET_SWITCH_TIME_CNT (uint16_t)(kFetSwitchTime_uS*(kTimerFreq_Khz/1000)) 
	 /**< kFetSwitchTime_uS converted to timer counts */
#define  MIN_TIMER_OCR_CNT   (uint16_t)(kMinTimerDelta_uS*(kTimerFreq_Khz/1000)) 
	 /**< kMinTimerDelta_uS converted to timer counts  */
#define  PWM_CYCLE_CNT	     (uint16_t)(kTimerFreq_Khz/kPwmFreq_Khz)	
     /**<Number of timer counts in one PWM cycle */

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
		//	TIMSK |= _BV(OCIE1A);
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
		pwmEntry_T *pIsrScriptEntry;   
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
		pIsrScriptEntry  = (pwmIsrData.isActiveTableA ? pwmIsrEntry.tableB : pwmIsrEntry.tableA);	
		
		//Setup First Entry State ePwmCommand_START. (Note this will never change)
		pIsrScriptEntry->command = ePwmCommand_START;
		pIsrScriptEntry->deltaTime = FET_SWITCH_TIME_CNT;
				
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
																
					uint16_t shortestDutyCycle = 65535;
						/**< This variable is used while we attempt to sort the pwmChannel entries. 
						 * it holds the shortest duty cycle it has found so far as we search the pwmArray. */
		
					pPwmChannel = pwmChannel; //Reset c pointer to first entry in pwmChannel array;
					
					//Find "nextPwmIndex" which is the channel with the lowest pulse width not used yet
					for (uint8_t n = 0;n<ePwmChannel_COUNT;n++)	{						
						if (pPwmChannel->dutyCycle < shortestDutyCycle && !pPwmChannel->used) {
							currentPwmIndex = n;
							shortestDutyCycle = p->dutyCycle;
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
					pIsrScriptEntry->command = command;
					pIsrScriptEntry->deltaTime  =  ((uint32_t) c->dutyCycle * PWM_CYCLE_CNT) / PWM_CONTROL_FULL_SCALE_CNTS;
										
					//---------------------------------------------------------------------------------
					// LOW SIDE FET ON ENTRY - TURN ON LOW SIDE FET FOR THIS CHANNEL
					//---------------------------------------------------------------------------------
					pIsrScriptEntry++; //Step to next  ISR script entry.
					pIsrScriptEntry->command = command + 1; 
						/* IN enum, the command for turning on low side is always immediately after
						 * the one for turning off the high side so we can +1 rather than using another
						 * case statement.*/
					pIsrScriptEntry->deltaTime = FET_SWITCH_TIME_CNT;
					
					
		} //End For entryPair
		
		
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


