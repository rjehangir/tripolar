/*
 * bldcPwm.h
 *
 * Created: 5/12/2015 4:03:34 PM
 *  Author: Alan
 */ 


#ifndef BLDCPWM_H_
#define BLDCPWM_H_






/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& USER CONFIGURATION
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/	


	#define kDutyCycleFullScale  1023
			/**< Upper scale for duty cycle specification. For the set PWM method, this number is the 100% 
			 * duty cycle equivelent. The set_pwm method will accept duty cycles between 0 and this number,
			 * where kDutyCycleFullScale is 100% duty cycle.												*/
			
	#define  kFetSwitchTime_uS 2
			/**< Fet Turn on Time in micro-seconds. FETS do not switch on or off instantly. There will be a
			 * delay from the time that we initiate turning off a fet, to the time that the FET is truly off.
			 * Since the we running half H bridges, a condition where both the high and low side FET are 
			 * turned on will create a short from power to ground. If we switched the High and Low side FETs
			 * at exactly the same time, the turn off delay would create a time window where both FETs are on
			 * at the same time. To prevent this, will implement kFetSwitchTime_nS delay (in micro-seconds) 
			 * between the time that we turn one fet on an h bridge off and the time that we turn the other
			 * FET (on the same HBridge) on.																*/
			
#define  kMinTimerDelta_uS  20
			/**< Minimum Time Delta required for PWM ISR Exit in micro seconds. 
			 * When we are running the PWM ISR, we will be processing timer interrupts. We will also be 
			 * setting the next timer expiration from within the ISR. If the next timer interrupt is 
			 * set too close to the current time, we could have issues where we might might miss the next
			 * interrupt while we are busy setting it up. To prevent this, we set a rule which says that 
			 * if we are within MIN_TIMER_OCR_US from the next timer expire, we will remain in the ISR
			 * to wait for the next event, rather than risk leaving the ISR.								*/
			
#define ISR_LOOP_US 12
			/**< When deltaTime is below this value, the ISR will loop too handle the next edge without 
			 * exiting the ISR, and without waiting for the timer to expire.								*/
			
#define PWM_FREQ_KHZ 1
			/**< Pwm Frequency in Tenths of A KiloHertz.
			 *  When we are running the PWM ISR, we will be processing timer interrupts. We will also be 
			 * setting the next timer expiration from within the ISR. If the next timer interrupt is 
			 * set too close to the current time, we could have issues where we might might miss the next
			 * interrupt while we are busy setting it up. To prevent this, we set a rule which says that 
			 * if we are within MIN_TIMER_OCR_US from the next timer expire, we will remain in the ISR
			 * to wait for the next event, rather than risk leaving the ISR.								*/
			
#define COIL_RATIO  7  
			/**< The number of sine cycles each coil needs to go through for motor to 
			 * make on rotation																				*/
						
#define PWM_TIMER_FREQ_KHZ  (uint16_t)16000
			/**< The frequency which timer1 (the 16 bit timer) is running at. If you change the prescaling
			 * or the clock frequency such that the timer frequency changes, you will need to change this 
			 * number.																						*/		




/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/	


#define  FET_SWITCH_TIME_CNT ((uint16_t)(kFetSwitchTime_uS*(PWM_TIMER_FREQ_KHZ/1000)) )
	 /**< kFetSwitchTime_uS converted to timer counts */
#define  MIN_TIMER_OCR_CNT   ((uint16_t)(kMinTimerDelta_uS*(PWM_TIMER_FREQ_KHZ/1000)) )
	 /**< kMinTimerDelta_uS converted to timer counts  */
	 
	 
#define ISR_LOOP_CNT  ((uint16_t)(ISR_LOOP_US*(PWM_TIMER_FREQ_KHZ/1000)) )

#define  PWM_CYCLE_CNT	    ((uint16_t)((PWM_TIMER_FREQ_KHZ)/(PWM_FREQ_KHZ))	)
     /**<Number of timer counts in one PWM cycle */

#define MAX_LOWX_CNT		( (uint16_t)(PWM_CYCLE_CNT - FET_SWITCH_TIME_CNT -1))
	/**<Maximum absolute time the ePwmCommand_LOWx commands are allowed. Longer than this they conflict
	 * with the ePwmCommand_ALLOFF command. Corresponds to timer counts since PWM cycle began*/
	
#define MAX_OFFX_CNT		((uint16_t)(MAX_LOWX_CNT - FET_SWITCH_TIME_CNT))
	/**<Maximum absolute time the ePwmCommand_OFFx commands are allowed. Longer than this they conflict
	 * with the ePwmCommand_ALLOFF command. Corresponds to timer counts since PWM cycle began*/	
	
/********************************************************************************************************/
/* CLASS: bldcPwm																						*/
/** 3 Channel Pulse Width Modulator designed specifically for brushless DC Motors. 
 *																										*/
/********************************************************************************************************/
class bldcPwm
{
	
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PUBLIC STRUCTURE DEFINITIONS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	public:
	
			
	
	
		/************************************************************************************************/
		/*  ENUM: pwmChannels_E																			*/
		/** Identifies array elements for PWM channel in pwmChannel array.								*/
		/************************************************************************************************/		
			typedef enum pwmChannels_E
			{
				ePwmChannel_A = 0,
				ePwmChannel_B,
				ePwmChannel_C,
				ePwmChannel_COUNT					
			}pwmChannels_T;	


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
			
			

		
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PUBLIC PROPERTIES
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	public:

	
		
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
			
			
			inline void tickle(void) { if (_updateOutstanding) updateISR();}
			/**< This function needs to be called on a regular basis to enable the this class to do
			 * housekeeping. Primarily, this is used to update the ISR if an update was made before
			 * but the ISR was busy and could not accept a new value.									 */
			 /*------------------------------------------------------------------------------------------*/
			 
		
			inline void update(void) {_updateOutstanding = true; updateISR();}
			/**< Updates the ISR to use the latest PWM duty cycle setpoints. This must be called after
			 * you have changed the pwm setpoints. You may change all 3 setpoints, and then call this 
			 * method once. The PWM output will not change until this method is called.					*/
			/*------------------------------------------------------------------------------------------*/				
			
			inline void set_pwm(pwmChannels_T channel, int16_t value)
			/**< Used to set the PWM duty cycle for any of the 3 pwm channels. Each channel corresponds to the 
			 * 3 coils on the brushless DC's motor.
			 * @param channel
			 *		The channel to set, See pwmChannels_T for more information.
			 * @param value		
			 *		The PWM pulse width to set. Value will be between 0 and kDutyCycleFullScale where
			 *		the high end of the range corresponds to 100% duty cycle.							*/
   		    /*------------------------------------------------------------------------------------------*/
			 { 
					_pwmChannel[channel].dutyCycle = value;
				
					
			}
			
			 bool busy(void);
			/**< Indicates if the ISR has not loaded the previous value
			 * @return true if the ISR can not accept a new PWM value. */
			
			 void isrEnable(bool isEnabled);
			
		
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE STRUCTURE DEFINITIONS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/ private:
								
		
			/************************************************************************************************/
			/* STRUCT: pwmChannelEntry_S																	*/
			/**<	Used to hold the PWM Duty cycle for each pwm channel. In addition to storage
			 * of the PWM value itself, it includes additional members which assist during the 
			 * processing of these values into the ISR data structure				   						*/	
			/************************************************************************************************/
			typedef struct pwmChannelEntry_S
			{
				uint16_t dutyCycle; 
					/**< PWM Duty Cycle for Channel. Range is from 0 to PWM_CONTROL_FULL_SCALE_CNTS
						* where a value of PWM_CONTROL_FULL_SCALE_CNTS indicates 100% pulse width.			*/
					
				uint16_t timerCount;
					/**< The number of timer counts that the channel is turned on during the pwm cycle.
					 * we precalculate it and put it here to simplfy the update routine. */
				bool used; 
					/**< This is used during sorting to indicate whether this entry was already already
						* used on a previous sort so that we dont reuse the entry on the next sort.			*/			
			}pwmChannelEntry_T;								 
			
			
			/************************************************************************************************/
			/* STRUCT: pwmSortList_S  																	*/
			/**<	Holds an entry for the preliminary list of pwm timer events. Includes 
			 * additional members to assist with the sorting processes.										*/
			/************************************************************************************************/
			typedef struct pwmSortList_S
			{
				pwmCommand_T command;  
					/**< What behavior to execute when the timer expires. See definition of pwmCommmand_T	*/
				uint16_t absoluteCount; 
					/**< The time from the start of the PWM cycle that the event is supposed to take place
						* measured in timer counts. This is different from the deltaTime used in pwmEntry_S
						* in that it is absolute time, rather than a delta from the previous event.			*/
				struct pwmSortList_S *pNextEntry;
					/**< This is used for sorting of the list, it is the array index of the entry which 
						* is next in the sort order. 0 Indicates its the last entry in list.				*/												
			}pwmSortList_T;
					
					
					

	
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE PROPERTIES
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:
	
			 pwmChannelEntry_T _pwmChannel[ePwmChannel_COUNT];
				/**< Holds non ISR data related to controlling the PWM channels. Each element in the array 
				 * corresponds to a pwm channel as indexed by pwmChannels_T. The user sets each of these element
				 * to configure the pwm. He then calls setPwmIsr() which will read this data and setup pwmIsrData  */								
			bool _updateOutstanding;
				/**< If true, indicates that one of the pwm values have been changed, but has not
				 * been updated in the ISR yet. */
						
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&&  USER CHANGEABLE CONSTANTS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	public:	  //Some of these need to be accessed by the ISR, so we need to be public here.
				
																				
			
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE METHODS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	
		private:
		
		
		inline uint16_t pwmDuration_cnt(uint16_t value)
		/**< returns the pwm duration in timer counts. This is the amount of time that the pulse
		 *  will remain on during the PWM cycle.
		 *  @param value 
		 *		The pwm channel duty cycle. Same units as set_pwmChannel
		 * @return 
		 *		The number of timer counts that the selected channel will remain on 
		 *      during the PWM cycle.																*/
		/*------------------------------------------------------------------------------------------*/
		{
			return ((int32_t)PWM_CYCLE_CNT * (int32_t)value  ) / kDutyCycleFullScale;			
		}
		
		
		
		 void updateISR(void);
		/* Takes the _pwmChannel[] data and updates the data structure which feeds the ISR
		 * so that these new PWM values are refected at the begining of the next cycle				*/
		/*------------------------------------------------------------------------------------------*/
		
	
	
	
};



#endif /* BLDCPWM_H_ */