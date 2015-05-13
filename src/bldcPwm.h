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
&&& ISR PROTOTYPES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/	
	
/********************************************************************************************************/
/* CLASS: bldcPwm
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
		
		
			void update(void);
			/**< Updates the ISR to use the latest PWM duty cycle setpoints. This must be called after
			 * you have changed the pwm setpoints. You may change all 3 setpoints, and then call this 
			 * method once. The PWM output will not change until this method is called. 
			/*------------------------------------------------------------------------------------------*/		 
			
			inline void set_pwm(pwmChannels_T channel, int16_t value) { _pwmChannel[channel].dutyCycle = value;}
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
	&&& PRIVATE STRUCTURE DEFINITIONS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/ private:

		

		
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
		
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE PROPERTIES
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:
	
			 pwmChannelEntry_T _pwmChannel[ePwmChannel_COUNT];
				/**< Holds non ISR data related to controlling the PWM channels. Each element in the array 
				 * corresponds to a pwm channel as indexed by pwmChannels_T. The user sets each of these element
				 * to configure the pwm. He then calls setPwmIsr() which will read this data and setup pwmIsrData  */								
						
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&&  USER CHANGEABLE CONSTANTS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	public:	  //Some of these need to be accessed by the ISR, so we need to be public here.
				
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
			
		static const uint16_t kTimerFreq_Khz = 16000;
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