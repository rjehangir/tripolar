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
#define F_CPU 16000000UL // 16 MHz
#include <util/delay.h>
#include <stdbool.h>

//#define DO_DEBUG
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





/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& STRUCTURES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	
			/************************************************************************************************/
			/* ENUM: isrExitMode_E																			*/
			/** Specifies the action to take when exiting the pwm ISR.										*/
			/************************************************************************************************/
			typedef enum isrExitMode_E
			{
				isrExitMode_Exit, ///< Exit ISR when completed, and handle next edge after reentry.
				isrExitMode_Wait, ///< Do not exit ISR,but stop and wait for timer to expire, then loop to beginning.
				isrExitMode_Loop  ///< Do not exit ISR and loop immediately to handle next edge without waiting.
			}pwmExitMode_T;
	

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
						* as a delta from the time that the previous command was executed.					*/	
				//bool waitInISR; 
					/**<set to true if deltaTime is so small that we risk missing the next ISR.					
						* If set to true, the ISR will not exit on the previous entry, and instead,
						* wait within the ISR for the next time to occur.									*/
					
				pwmExitMode_T exitMode; //Controls completion behavior at end of ISR. See pwmExitMode_T.
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
						*  DEFAULT: false																	*/		
				//uint16_t startTime; //Timer value when 	

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
	  
	  const pwmEntry_T pwmInit[8] =  {		  
									  {(bldcPwm::pwmCommand_T) 0,	1600,	isrExitMode_Exit},
									  {(bldcPwm::pwmCommand_T)5,	1600,	isrExitMode_Exit},
									  {(bldcPwm::pwmCommand_T)6,	1600,	isrExitMode_Exit},
									  {(bldcPwm::pwmCommand_T)3,	1600,	isrExitMode_Exit},
									  {(bldcPwm::pwmCommand_T)4,	1600,	isrExitMode_Exit},
									  {(bldcPwm::pwmCommand_T)1,	1600,	isrExitMode_Exit},
									  {(bldcPwm::pwmCommand_T)2,	1600,	isrExitMode_Exit},
									  {(bldcPwm::pwmCommand_T)7,	1600,	isrExitMode_Exit}
								  };

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTION PROTOTYPES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

bool checkISRData(pwmEntry_T  *table);		  

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
		OCR1A = PWM_CYCLE_CNT;  //Allow us to count freely so we know how long we are in ISR
 		DEBUG_OUT(0x08);
		redOn();
		bool incEntry = true; //When true, ISR will increment the pwmIsrData.pEntry pointer before exiting.
		bool orderError = false;
		bool doExit = false;
		if (!pwmIsrData.enabled) return;
		for (int i=0;i<10;i++) {	//Repeat up to 11 times if deltaTime keeps being too short		
			DEBUG_OUT(0x09);
			
			//DEBUG_OUT(pwmIsrData.pEntry->command);
		
		
			DEBUG_OUT(pwmIsrData.pEntry->command);
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
				//	_delay_ms(10);
					
						AnFETOn();					
						incEntry = true;
					
					break;
				case bldcPwm::ePwmCommand_OFFB:
					BpFETOff();
					incEntry = true;
					break;
				case bldcPwm::ePwmCommand_LOWB:
				//	_delay_ms(10);
					
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
					AnFETOff();
					BnFETOff();
					CnFETOff();
													
					if (pwmIsrData.changeTable == true)  //If user has requested a change of tables then ...
					{
						DEBUG_OUT(0x0A);
						//_delay_us(200);
						pwmIsrData.pTableStart = (pwmIsrData.isActiveTableA ? pwmIsrData.tableA : pwmIsrData.tableB);										
							/* Go to the beginning of the next table */
						pwmIsrData.changeTable = false;															
							/* In theory, the user sets changeTable to force a change in the table, in reality
							 * isActiveTableA is enough. However, the user will look at changeTable to see if 
							 * the change over was made, so that he knows when he can start writing to the 
							 * free table again. so we reset the flag here.									*/
					}
					//pwmIsrData.pEntry = pwmIsrData.tableA;
					pwmIsrData.pEntry = pwmIsrData.pTableStart; //Reset script entry to beginning.
					incEntry = false; //Don't increment entry because we just sent entry to beginning instead.
					break;				
				default:
					DEBUG_OUT(0xFF);
					//Something is very wrong, stop processing the ISR
					pwmIsrData.enabled	 = false;
					break;					
			}
			
			
			
			
						
			//Go to next entry if the switch told us to.
			if (incEntry) pwmIsrData.pEntry++;
			
			if (pwmIsrData.pEntry->exitMode != isrExitMode_Loop)
			{
				OCR1A = pwmIsrData.pEntry->deltaTime;  //Configure the time of the next interrupt.
				if (pwmIsrData.pEntry->exitMode  == isrExitMode_Exit)
				{
					
					//TCNT1 = 0;
					//TIFR = _BV(OCF1A); // Clear any pending interrupts
					break;
				}
				else
				{
					
					//while (TCNT1 <  pwmIsrData.pEntry->deltaTime ) asm(" ");	//Otherwise, stay in ISR and wait for the next timer expiration.
					while ((TIFR & _BV(OCF1A)) == 0)  asm(" ");
					OCR1A = PWM_CYCLE_CNT;  //Allow us to count freely so we know how long we are in ISR
					TIFR = _BV(OCF1A); // Clear any pending interrupts
					
				}
			}
			TCNT1 = 0;
	
		}
		
		//   <========== !!!!!!! CLEAR THE INTERRUPT SOURCE !!!!!!!!
		
	redOff();	
	DEBUG_OUT(0x0B);
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
		
		//Use temporary table in tableA for now...
			pwmIsrData.pTableStart = pwmIsrData.tableA;
			pwmIsrData.isActiveTableA = true; 
			pwmIsrData.pEntry =  pwmIsrData.tableA;
			pwmIsrData.changeTable =  false;
			pwmIsrData.enabled =  true;
			
			memcpy((void *)pwmIsrData.tableA,(const void *)pwmInit,sizeof(pwmInit));					
			for (uint8_t n=0;n<3;n++) _pwmChannel[n].dutyCycle = 0;		
			_updateOutstanding = false;																			
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
		
	//	boardInit(); //Part of ESC include file, sets up the output pins.
		lowSideOff();
		highSideOff();
		
		// Reset timer
		TCCR1A = 0;
		TCCR1B = 0;
		TIMSK = 0;
		TCNT1 = 0;
		
		
		OCR1A = 100*PWM_TIMER_FREQ_KHZ;	
			/* Set compare match register to overflow at 100ms, which means 
			   the PWM will have its first interrupt in 100ms..
			   Which means PWM starts in 100ms		*/
		TCCR1B |= _BV(WGM12); // Turn on CTC mode for timer 1				
		TCCR1B |= _BV(CS10); // Set for no prescaler (Timer Freq = 16Mhz)		
		TIMSK |= _BV(OCIE1A); // Enable timer compare interrupt		
		TIFR = _BV(OCF1A); // Clear any pending interrupts		
		sei();					
	//	_delay_ms(100);
	}



	 	 
	/****************************************************************************
	*  Class: bldcPwm
	*  Method: updateISR
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	void  bldcPwm::updateISR(void)
	{
		
	
		//if (busy()) return;	
		
		
		//uint8_t sreg = SREG;
		//cli();
		
		if (pwmIsrData.changeTable) return;
		DEBUG_OUT(0x0E);
		

		static pwmSortList_T sortList[8];
			/**< A temporary list which information on each ISR command and its absolute
			 * time. Includes addition fields which support list sorting. A list of commands
			 * is made here first, sorted, and then finally exported to the ISR data structure. 
			 * it is made 'static' to prevent it from being put on the stack, which is too small
			 * to hold it.*/
		pwmSortList_T *pSortEntry;
		pwmSortList_T *pPreviousEntry;
		
		uint8_t linkPosition = 0;
		
		
		volatile pwmEntry_T *pIsrScriptEntry;   
			/**< Pointer to the an entry in pwm script table which we are creating.
			 * We use this point to navigate the table as we populate it. */		

		uint16_t totalTime = 0; 
			/**<Running count of what time elapsed is since the start of the PWM cycle, as
             * of the last pIsrScriptEntry.  We do this to help convert absolute time into
             * deltaTime while populating the ISR data structure. Using are in counts. */
		
						
		DEBUG_OUT(0x01);
		for (uint8_t n=0;n<=6;n++)
		{
			sortList[n].command = (pwmCommand_T)n;
			sortList[n].pNextEntry = &sortList[n+1];
		}
		
		//Last entry is a unique case because the next entry pointer is null.
		sortList[7].pNextEntry = 0; //Mark the end of the linked list.
		sortList[7].command = ePwmCommand_ALLOFF;
		
		DEBUG_OUT(0x02);
		
		//---------------------------------------------------------------------------------
		// COVVERT FROM DUTY CYCLE TO TIMER EXPIRATION
		//---------------------------------------------------------------------------------				
		#define MAX_PWM_CHANNEL PWM_CYCLE_CNT - (2*FET_SWITCH_TIME_CNT)
		for (uint16_t channel = 0; channel <3;channel++)
		{
			uint16_t timerCount = pwmDuration_cnt(_pwmChannel[channel].dutyCycle);					
			timerCount = (timerCount >=MAX_PWM_CHANNEL? MAX_PWM_CHANNEL-1:timerCount);					
			_pwmChannel[channel].timerCount = timerCount;
		}
		
		DEBUG_OUT(0x03);
		

		//---------------------------------------------------------------------------------
		// POPULATE THE ABSOLUTE TIMES FOR START AND ALLOFF
		//---------------------------------------------------------------------------------
		sortList[ePwmCommand_START].absoluteCount = 0;
		sortList[ePwmCommand_ALLOFF].absoluteCount = PWM_CYCLE_CNT - FET_SWITCH_TIME_CNT;
		
		//---------------------------------------------------------------------------------
		// POPULATE THE ABSOLUTE TIMES FOR EACH CHANNEL'S ePwmCommand_OFFx, ePwmCommand_LOWx
		//---------------------------------------------------------------------------------	
		sortList[ePwmCommand_OFFA].absoluteCount = _pwmChannel[ePwmChannel_A].timerCount;
		sortList[ePwmCommand_LOWA].absoluteCount = _pwmChannel[ePwmChannel_A].timerCount + FET_SWITCH_TIME_CNT;		
		sortList[ePwmCommand_OFFB].absoluteCount = _pwmChannel[ePwmChannel_B].timerCount;
		sortList[ePwmCommand_LOWB].absoluteCount = _pwmChannel[ePwmChannel_B].timerCount + FET_SWITCH_TIME_CNT;		
		sortList[ePwmCommand_OFFC].absoluteCount = _pwmChannel[ePwmChannel_C].timerCount;
		sortList[ePwmCommand_LOWC].absoluteCount = _pwmChannel[ePwmChannel_C].timerCount + FET_SWITCH_TIME_CNT;		
				
				
		DEBUG_OUT(0x04);
				
		//---------------------------------------------------------------------------------
		// SORT THE LIST
		//---------------------------------------------------------------------------------
		/*Note that the first and last items in the list are fixed and don't need to 
		 * be sorted. ePwmCommand_START is always first, and ePwmCommand_ALLOFF is 
		 * always last. The consequence of this is: 1) We never have to change 
		 * the PHeadEntry pointer (I trust that the compiler will optimize this out)
		 * and 2) That we never have to check for insertion after the last entry. */

		bool touchedFlag; ///<true if the list order was changed at all	
		do {
			DEBUG_OUT(0x05);
			linkPosition = 0;
			pSortEntry = sortList[0].pNextEntry; 
				/* Skipping first entry, The head is always the first element. See note above. */
			pPreviousEntry = sortList; //Since current entry is seconds element, previous is first
				
		    touchedFlag = false; //true if the list order was changed at all
			do {
				DEBUG_OUT(0x06);				
				if (pSortEntry->absoluteCount > pSortEntry->pNextEntry->absoluteCount) {	
					
					pwmSortList_T* pTemp =pSortEntry->pNextEntry->pNextEntry;
					pPreviousEntry->pNextEntry = pSortEntry->pNextEntry; //Previous Entry now points to guy after me
					pSortEntry->pNextEntry->pNextEntry = pSortEntry;     //guy after me points to me
					pSortEntry->pNextEntry = pTemp;						 //I point to what guy after me WAS pointing too					
					touchedFlag = true;	 //We made a change, so this sort needs to run at least once more
					linkPosition++;  
				}
				
				pPreviousEntry = pSortEntry;				
			    pSortEntry = pSortEntry->pNextEntry;  //increment to next entry				
				linkPosition++;

				
			}while (linkPosition < 6 );
				/* Remember that the last position has a pNextEntry of 0.
				 * If the we are at the second to last list position, no need to 
				 * compare it with the last because by definition, nothing comes 
				 * after the ePwmCommand_ALLOFF which was already placed there. */

		} while (touchedFlag);
			
			
	
				
		//---------------------------------------------------------------------------------
		// LOAD THE LIST INTO THE ISR DATA STRUCTURE
		//---------------------------------------------------------------------------------	
		pIsrScriptEntry  = (pwmIsrData.isActiveTableA ? pwmIsrData.tableB : pwmIsrData.tableA);	
		pwmEntry_T *tableHead  = pIsrScriptEntry;
		
		
		//Start is a special case  because we want to make the deltaTime
		//FET_SWITCH_TIME_CNT rather than the 0 found in the sortList
		//To make it easy,. just do it manually before we run the loop
		//where we will then skip it.
		pIsrScriptEntry->command = ePwmCommand_START;
		pIsrScriptEntry->deltaTime = FET_SWITCH_TIME_CNT;

		
		//pIsrScriptEntry++;	//Move to second entry
		
		pSortEntry = (pwmSortList_T *)sortList; //Reset pointer to first entry
		
		for (int n=0;n<8;n++)
		{	
			if (n!=0){
				pIsrScriptEntry->command = (pwmCommand_T)pSortEntry->command;
				pIsrScriptEntry->deltaTime = pSortEntry->absoluteCount - totalTime;
			}
			
			if (pIsrScriptEntry->deltaTime <= ISR_LOOP_CNT ) pIsrScriptEntry->exitMode = isrExitMode_Loop;
			else if (pIsrScriptEntry->deltaTime <= MIN_TIMER_OCR_CNT ) pIsrScriptEntry->exitMode = isrExitMode_Wait;
			else pIsrScriptEntry->exitMode = isrExitMode_Exit;

			
			
									
			totalTime += pIsrScriptEntry->deltaTime;
			pIsrScriptEntry++;	
			pSortEntry = pSortEntry->pNextEntry;					
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
		_updateOutstanding = false;	
		if  (!checkISRData(tableHead))
		{
			DEBUG_OUT(0x0D);
			asm("NOP");
				if  (!checkISRData(tableHead)) asm("NOP");  //For debug so we can step and see why it failed.
		}
		
		DEBUG_OUT(0x0E);
	}
	
	
	
	/*****************************************************************************
	*  Function: checkISRData
	*	Description:															 */
   /**		Looks at the tableA or tableB structure and checks if the
	*       array holds valid data.
	* @param isTableA true if tableA, false if tableB 
	* @return true if data is valid. False if problem 
	****************************************************************************/	
	bool checkISRData(pwmEntry_T  *table)	
	{	
		bool retVal = false;
		bool commandCalled[8];
		pwmEntry_T *p = table;
		uint32_t totalCounts = 0;
		
		for (uint8_t n=0;n<8;n++) commandCalled[n]=false;
		
		for (uint8_t n=0;n<8;n++)
		{
			if (p->command ==  bldcPwm::ePwmCommand_LOWA && commandCalled[bldcPwm::ePwmCommand_OFFA] == false) 
					return false;
			if (p->command == bldcPwm::ePwmCommand_LOWB && commandCalled[bldcPwm::ePwmCommand_OFFB] == false) 
				return false;
			if (p->command == bldcPwm::ePwmCommand_LOWC && commandCalled[bldcPwm::ePwmCommand_OFFC] == false) 
				return false;
			totalCounts += p->deltaTime;
			
			commandCalled[p->command ] = true;
			p++;
		}
		
		
		if (totalCounts > PWM_CYCLE_CNT) 
				return false;
		if (totalCounts < (PWM_CYCLE_CNT*2)/3)
				return false;
		if (table[0].command != bldcPwm::ePwmCommand_START)
			 return false;
		if (table[7].command != bldcPwm::ePwmCommand_ALLOFF) 
			return false;
		return true;
	}
	
	
	
         
	/****************************************************************************
	*  Class: bldcPwm
	*  Method: busy
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/	
	bool bldcPwm::busy(void)
	{
		bool retVal;	
		uint8_t sregVal = SREG;			
		cli();
		retVal = pwmIsrData.changeTable;
		SREG = sregVal;
		return retVal;
				
	}
	
	
         
	/****************************************************************************
	*  Class: bldcPwm
	*  Method: enable
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/	
	void bldcPwm::isrEnable(bool isEnable)
	{
		pwmIsrData.enabled = isEnable;
	}