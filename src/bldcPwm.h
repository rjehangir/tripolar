/*
 * bldcPwm.h
 *
 * Created: 5/12/2015 4:03:34 PM
 *  Author: Alan
 */ 


#ifndef BLDCPWM_H_
#define BLDCPWM_H_

/********************************************************************************************************/
/* CLASS: bldcPwm
/** 3 Channel Pulse Width Modulator designed specifically for brushless DC Motors. 
 *																										*/
/********************************************************************************************************/
class bldcPwm
{
	
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE STRUCTURE DEFINITIONS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/
		private:

			
		/************************************************************************************************/
		/* ENUM: pwmCommand_E																			*/
		/** Specifies commands which the pwmISR can execute.
		 *	In the pwm timer interrupt routine, we define timer expirations, and things to do when 
		 *  that timer expires. This enumerated type is used to command what happens when that
		 *  timer expires.																				*/
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
			}pwmCommmand_T;
		
		
		
		/************************************************************************************************/
		/* STRUCT: pwmEntry_S																			*/
		/** This contains information to define a single timer expiration for the pwm ISR. An array
	     * of this structure is enough to completely describe a pwm cycle.								*/
		/************************************************************************************************/
			typedef struct pwmEntry_S
			{
				pwmCommand_T command;  
					/**< What behavior to execute when the timer expires. See definition of pwmCommmand_T	*/
				int16_t	deltaTime; 
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
		
		
		/************************************************************************************************/
		/* STRUCT: pwmChannelEntry_S																	*/
		/**<																							*/	
		/************************************************************************************************/
			typedef struct pwmChannelEntry_S
			{
				uint16_t dutyCycle; 
					/**< PWM Duty Cycle for Channel. Range is from 0 to PWM_CONTROL_FULL_SCALE_CNTS
					 * where a value of PWM_CONTROL_FULL_SCALE_CNTS indicates 100% pulse width.				*/
				bool used; 
					/**< This is used during sorting to indicate whether this entry was already already
					 * used on a previous sort so that we dont reuse the entry on the next sort.			*/			
			}pwmChannelEntry_T;
		
		
		
		/************************************************************************************************/
		/*  ENUM: pwmChannels_E
		/** Identifies array elements for PWM channel in pwmChannel array.								*/
		/************************************************************************************************/		
			typedef enum pwmChannels_E
			{
				ePwmChannel_A = 0,
				ePwmChannel_B,
				ePwmChannel_C,
				ePwmChannel_COUNT					
			}pwmChannels_T;
		
/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PUBLIC PROPERTIES
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:
		
		
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PUBLIC METHODS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	public:
		
			bldcPwm(void);
			/**<Instatiator. Called automatically when the class is instantiated. Good place for class
			 * property initialization																	*/
			/*------------------------------------------------------------------------------------------*/
		

			void begin(void);
			/**< Setup method for class. Call this after the class is instantiated, but before using 
				* the class.																			*/
			/*------------------------------------------------------------------------------------------*/
		
		
			void update(void);
			/**< Updates the ISR to use the latest PWM duty cycle setpoints. This must be called after
			 * you have changed the pwm setpoints. You may change all 3 setpoints, and then call this 
			 * method once. The PWM output will not change until this method is called. 
			/*------------------------------------------------------------------------------------------*/		 
			
			inline void set_pwm(pwmChannels_T channel, int16_t value) { pwmChannel[channel].dutyCycle = value);}
			/**< Used to set the PWM duty cycle for any of the 3 pwm channels. Each channel corresponds to the 
			 * 3 coils on the brushless DC's motor.
			 * @param channel
			 *		The channel to set, See pwmChannels_T for more information.
			 * @param value		
			 *		The PWM pulse width to set. Value will be between 0 and kDutyCycleFullScale where
			 *		the high end of the range corresponds to 100% duty cycle.							*/
   		    /*------------------------------------------------------------------------------------------*/
			 
		 
		
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE PROPERTIES
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:
	
			pwmChannelEntry_T pwmChannel[ePwmChannel_COUNT];
				/**< Holds non ISR data related to controlling the PWM channels. Each element in the array 
				 * corresponds to a pwm channel as indexed by pwmChannels_T. The user sets each of these element
				 * to configure the pwm. He then calls setPwmIsr() which will read this data and setup pwmIsrData  */
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE USER CHANGEABLE CONSTANTS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:	
				
		static const uint16_t kDutyCycleFullScale = 1023;
			/**< Upper scale for duty cycle specification. For the set PWM method, this number is the 100% 
			 * duty cycle equivelent. The set_pwm method will accept duty cycles between 0 and this number,
			 * where kDutyCycleFullScale is 100% duty cycle.												*/
			
		static const uint8_t kFetSwitchTime_uS = 2;
			/**< Fet Turn on Time in micro-seconds. FETS do not switch on or off instantly. There will be a
			 * delay from the time that we initiate turning off a fet, to the time that the FET is truly off.
			 * Since the we running half H bridges, a condition where both the high and low side FET are 
			 * turned on will create a short from power to ground. If we switched the High and Low side FETs
			 * at exactly the same time, the turn off delay would create a time window where both FETs are on
			 * at the same time. To prevent this, will implement kFetSwitchTime_nS delay (in micro-seconds) 
			 * between the time that we turn one fet on an h bridge off and the time that we turn the other
			 * FET (on the same HBridge) on.																*/
			
		static const uint8_t kMinTimerDelta_uS = 5;
			/**< Minimum Time Delta required for PWM ISR Exit in micro seconds. 
			 * When we are running the PWM ISR, we will be processing timer interrupts. We will also be 
			 * setting the next timer expiration from within the ISR. If the next timer interrupt is 
			 * set too close to the current time, we could have issues where we might might miss the next
			 * interrupt while we are busy setting it up. To prevent this, we set a rule which says that 
			 * if we are within MIN_TIMER_OCR_US from the next timer expire, we will remain in the ISR
			 * to wait for the next event, rather than risk leaving the ISR.								*/
			
		static const uint8_t kPwmFreq_Khz = 2;
			/**< Pwm Frequency in KiloHertz.
			 *  When we are running the PWM ISR, we will be processing timer interrupts. We will also be 
			 * setting the next timer expiration from within the ISR. If the next timer interrupt is 
			 * set too close to the current time, we could have issues where we might might miss the next
			 * interrupt while we are busy setting it up. To prevent this, we set a rule which says that 
			 * if we are within MIN_TIMER_OCR_US from the next timer expire, we will remain in the ISR
			 * to wait for the next event, rather than risk leaving the ISR.								*/
			
		static const uint8_t kCoilRatio = 7;
			/**< The number of sine cycles each coil needs to go through for motor to 
			 * make on rotation																				*/
			
		static const uint16_t TIMER_FREQ_KHZ = 16000;
			/**< The frequency which timer1 (the 16 bit timer) is running at. If you change the prescaling
			 * or the clock frequency such that the timer frequency changes, you will need to change this 
			 * number.																						*/
			
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE METHODS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	
		private:
		
		

	
	
	
};



#endif /* BLDCPWM_H_ */