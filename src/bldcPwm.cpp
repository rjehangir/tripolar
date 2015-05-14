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

#include <stdbool.h>
#include "afro_nfet.h"
#include "fets.h"
#include <avr/io.h>
#include "bldcPwm.h"
#include <string.h>


#include <avr/interrupt.h>

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/


#define  FET_SWITCH_TIME_CNT (uint16_t)(bldcPwm::kFetSwitchTime_uS*(bldcPwm::kTimerFreq_Khz/1000)) 
	 /**< kFetSwitchTime_uS converted to timer counts */
#define  MIN_TIMER_OCR_CNT   (uint16_t)(bldcPwm::kMinTimerDelta_uS*(bldcPwm::kTimerFreq_Khz/1000)) 
	 /**< kMinTimerDelta_uS converted to timer counts  */
#define  PWM_CYCLE_CNT	     (bldcPwm::kTimerFreq_Khz/bldcPwm::kPwmFreq_Khz)	
     /**<Number of timer counts in one PWM cycle */
#define MAX_OFFX_CNT		 PWM_CYCLE_CNT - 2*FET_SWITCH_TIME_CNT-1
	/**<Maximum absolute time the ePwmCommand_OFFx commands are allowed. Longer than this they conflict
	 * with the ePwmCommand_ALLOFF command. Corresponds to timer counts since PWM cycle began*/
#define MAX_LOWX_CNT		 PWM_CYCLE_CNT - FET_SWITCH_TIME_CNT -1
	/**<Maximum absolute time the ePwmCommand_LOWx commands are allowed. Longer than this they conflict
	 * with the ePwmCommand_ALLOFF command. Corresponds to timer counts since PWM cycle began*/
	




/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STRUCTURES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	

			/************************************************************************************************/
			/* STRUCT: pwmEntry_S																			*/
			/** This contains information to define a single timer expiration for the pwm ISR. An array
		 	 * of this structure is enough to completely describe a pwm cycle.								*/
			/************************************************************************************************/
			typedef volatile struct pwmEntry_S
			{
				volatile bldcPwm::pwmCommand_T command;  
					/**< What behavior to execute when the timer expires. See definition of pwmCommmand_T	*/
				volatile uint16_t	deltaTime; 
					/**< What value the timer should be at when this command is executed. This is referenced
						* as a delta from the time that the previous command was executed.						*/	
				bool waitInISR; 
					/**<set to true if deltaTime is so small that we risk missing the next ISR.					
						* If set to true, the ISR will not exit on the previous entry, and instead,
						* wait within the ISR for the next time to occur.										*/
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
			typedef  struct pwmIsrData_S
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
					pwmEntry_T *pEntry;
					/**< Pointer to the current entry in the currently active table. This is used 
						* by the ISR to quickly access the table without having to do pointer multiplication
						* to find it's position in the current table.			 		
						* DEFAULT: tableA[0]																	*/
				volatile pwmEntry_T *pTableStart;   
					/**<Pointer to beginning of table the ISR is using. Either tableA or tableB depending 
						*  which table the ISR is selected to be using.										*/
				volatile bool enabled; 
					/**< When true, the ISR will run, when false, the ISR will return without 
						*	doing anything.		
						*  DEFAULT: false																		*/		
				uint16_t startTime; //Timer value when 		
			}pwmIsrData_T;									
				
									

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STATIC VARIABLES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	  pwmIsrData_T pwmIsrData;
		/**< Holds all information used by the pwm  timer interrupt service routine. 
		 * See the declaration of pwmIsrData_T for more information	 */
	  pwmEntry_T IsrCurrentEntry;

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
		bool incEntry = true; //When true, ISR will increment the pwmIsrData.pEntry pointer before exiting.
		if (!pwmIsrData.enabled) return;
		do {
			switch (pwmIsrData.pEntry->command)
			{
				case bldcPwm::ePwmCommand_START:
					ApFETOn();
					BpFETOn();
					CpFETOn();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_OFFA:
					ApFETOff();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_LOWA:
					AnFETOn();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_OFFB:
					BpFETOff();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_LOWB:
					BnFETOn();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_OFFC:
					CpFETOff();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_LOWC:
					CnFETOn();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_ALLOFF:
					lowSideOff();
					if (pwmIsrData.changeTable == true)  //If user has requested a change of tables then ...
					{
						pwmIsrData.pTableStart = (pwmIsrData.isActiveTableA ? pwmIsrData.tableA : pwmIsrData.tableB);										
							/* Go to the beginning of the next table */
						pwmIsrData.changeTable = false;															
							/* In theory, the user sets changeTable to force a change in the table, in reality
							 * isActiveTableA is enough. However, the user will look at changeTable to see if 
							 * the change over was made, so that he knows when he can start writing to the 
							 * free table again. so we reset the flag here.									*/
					}
					pwmIsrData.pEntry = pwmIsrData.pTableStart; //Reset script entry to beginning.
					incEntry = false; //Don't increment entry because we just sent entry to beginning instead.
					break;				
				default:
					//Something is very wrong, stop processing the ISR
					pwmIsrData.enabled	 = false;
					break;					
			}
			
			//Go to next entry if the switch told us to.
			if (incEntry) pwmIsrData.pEntry++;
			
			TIFR |= _BV(OCF1A); // Clear any pending interrupts 	
			OCR1A = pwmIsrData.pEntry->deltaTime;  //Configure the time of the next interrupt.
			if (! pwmIsrData.pEntry->waitInISR) break; //If the expiration time is not too close, then exit ISR	

			while (TCNT1 < pwmIsrData.pEntry->deltaTime ) asm(" ");	//Otherwise, stay in ISR and wait for the next timer expiration.		
				
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
		pwmIsrData.pTableStart = pwmIsrData.tableA;
		pwmIsrData.isActiveTableA = true;
		pwmIsrData.changeTable = false;
		pwmIsrData.pEntry =  pwmIsrData.tableA;
		pwmIsrData.pTableStart = pwmIsrData.tableA;
		pwmIsrData.enabled = false;
		
		for (uint8_t n=0;n<3;n++) _pwmChannel[n].dutyCycle = 0;
																					
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
		//Initialize ISR Data 
			pwmIsrData.pTableStart = pwmIsrData.tableA;
			pwmIsrData.isActiveTableA = true;
			pwmIsrData.changeTable = false;
			pwmIsrData.pEntry =  pwmIsrData.tableA;
			pwmIsrData.pTableStart = 	 pwmIsrData.tableA;
			pwmIsrData.enabled = false;
			
		update();  //Initialize the pwmIsrData.table
		
		// Reset timer
			TCCR1A = 0;
			TCCR1B = 0;
			TIMSK = 0;
			TCNT1 = 0;
		
		OCR1A = 100*kTimerFreq_Khz;	
			/* Set compare match register to overflow at 100ms, which means 
			   the PWM will have its first interrupt in 100ms..
			   Which means PWM starts in 100ms		*/
		TCCR1B |= _BV(WGM12); // Turn on CTC mode for timer 1				
		TCCR1B |= _BV(CS10); // Set for no prescaler (Timer Freq = 16Mhz)		
		TIMSK |= _BV(OCIE1A); // Enable timer compare interrupt		
		TIFR |= _BV(OCF1A); // Clear any pending interrupts
		
		pwmIsrData.enabled = true;
		sei();			
	}


	 	 
	/****************************************************************************
	*  Class: bldcPwm
	*  Method: update
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	void  bldcPwm::update(void)
	{
		
		uint16_t eventTimes[ePwmCommand_END_OF_ENUM];
			/*An array of the absolute times for the isrPwmCommands to occur, where 
			  the value held is the time from the start of the PWM cycle measured in 
			  timer counts (on timer1). The array indexes correspond to the pwmCommand_E
			  commands. For example array index 1 is the time that ePwmCommand_OFFA occurs.*/
			 			  
		uint8_t sortOrder[] = {0,1,2,3,4,5,6,7};
			/*This holds the order in which each of the commands should be called. The index
			  corresponds to the sort order (0 element holds the command which occurs first,
			  1 holds the command which occurs second, etc). The value is the command which
			  occurs in that order position.  This list starts unsorted, and this method
			  will eventually sort the list based on the corresponding eventTimes array 
			  entry. */		

		volatile pwmEntry_T *pIsrScriptEntry;   
			/**< Pointer to the an entry in pwm script table which we are creating.
			 * We use this point to navigate the table as we populate it. */		

		uint16_t totalTime = 0; 
			/**<Running count of what time elapsed is since the start of the PWM cycle, as
             * of the last pIsrScriptEntry.  We do this to help convert absolute time into
             * deltaTime while populating the ISR data structure. Using are in counts. */
						
		//---------------------------------------------------------------------------------
		// POPULATE THE ABSOLUTE TIMES
		//---------------------------------------------------------------------------------
		eventTimes[ePwmCommand_START] = 0;
		eventTimes[ePwmCommand_OFFA] = _pwmChannel[ePwmChannel_A].dutyCycle;
		eventTimes[ePwmCommand_LOWA] = _pwmChannel[ePwmChannel_A].dutyCycle + FET_SWITCH_TIME_CNT;		
		eventTimes[ePwmCommand_OFFB] = _pwmChannel[ePwmChannel_B].dutyCycle;
		eventTimes[ePwmCommand_LOWB] = _pwmChannel[ePwmChannel_B].dutyCycle + FET_SWITCH_TIME_CNT;
		eventTimes[ePwmCommand_OFFC] = _pwmChannel[ePwmChannel_C].dutyCycle;
		eventTimes[ePwmCommand_LOWC] = _pwmChannel[ePwmChannel_C].dutyCycle + FET_SWITCH_TIME_CNT;
		eventTimes[ePwmCommand_ALLOFF] = PWM_CYCLE_CNT - FET_SWITCH_TIME_CNT;
		
		//Dont allow any times to exceed the ePwmCommand_ALLOFF time.
		for (int n=1;n<ePwmCommand_ALLOFF;n+=2) if (eventTimes[n] >= MAX_OFFX_CNT) {
			eventTimes[n] = MAX_OFFX_CNT;
			eventTimes[n+1] = MAX_LOWX_CNT;			
		}
		
		//---------------------------------------------------------------------------------
		// SORT THE LIST
		//---------------------------------------------------------------------------------
		
		for (uint8_t cycles = 0;cycles<4;cycles++)
		{
			for (uint8_t n=ePwmCommand_OFFA; n < ePwmCommand_ALLOFF; n++)
			{
				if (eventTimes[sortOrder[n]] >  eventTimes[sortOrder[n+1]]) {
					uint8_t tempOrder = sortOrder[n+1];
					sortOrder[n+1] = sortOrder[n];
					sortOrder[n] = tempOrder;
				}
			}
		}
				
		//---------------------------------------------------------------------------------
		// LOAD THE LIST INTO THE ISR DATA STRUCTURE
		//---------------------------------------------------------------------------------	
		pIsrScriptEntry  = (pwmIsrData.isActiveTableA ? pwmIsrData.tableB : pwmIsrData.tableA);	
		
		for (int n=0;n<8;n++)
		{
			
			pIsrScriptEntry->command = (pwmCommand_T)sortOrder[n];
			pIsrScriptEntry->deltaTime = sortOrder[n] - totalTime;
			if (pIsrScriptEntry->deltaTime > MIN_TIMER_OCR_CNT ) pIsrScriptEntry->waitInISR = true;
			totalTime += sortOrder[n];
			pIsrScriptEntry++;						
		}
		
		//---------------------------------------------------------------------------------
		// TELL THE ISR TO SWITCH TO THE TABLE WE JUST CREATED
		//---------------------------------------------------------------------------------
		uint8_t sreg = SREG; //Save interrupt state
		cli(); //Turn off interrupts while we write to active part of ISR's data
		{
			pwmIsrData.changeTable = true;
			pwmIsrData.isActiveTableA = !pwmIsrData.isActiveTableA;
		}
		SREG = sreg; //Restore Interrupt State			
	}
	
	
	
	
