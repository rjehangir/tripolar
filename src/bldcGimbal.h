/*
 * bldcGimbal.h
 * Description: header file for the bldcGimbal class which is a driver for controlling a brushless
 *				dc motor as a gimble.
 * Created: 5/12/2015 4:03:34 PM
 *  Author: Alan
 */ 

#ifndef BLDCGIMBLE_H_
#define BLDCGIMBLE_H_
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INCLUDES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	#include <avr/io.h>
	#include "bldcPwm.h"
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& USER CONFIGURATION
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	#define COIL_RATIO  7  
				/**< The number of sine cycles each coil needs to go through for motor to 
				 * make on rotation		*/		
				
	
	/*
	---------------------------------------------------------------------------------------------------
	POWERSCALE SETTINGS
		The following settings control how we scale the power applied to the motor, relative to the 
		speed the motor is operating at. The gimbal control method always applies full power to the motor
		even when the motor is not turning. The power scaling feature allows power to be reduced 
		at low speeds in order to conserve power consumption. 
	---------------------------------------------------------------------------------------------------
	*/
			#define POWER_FULL_SCALE 100
				/*The _PowerScale parameter controls what percentage of power is provided to the coils.
				  This value sets what number corresponds to 100 percent power. For example, if this
				  were set to 10, power scale would be specified in a range of 1 to 10. */
		
			/*
			---------------------------------------------------------------------------------------------------		
			POWER PROFILE LINES
				The power profile is defined by 2 slopes. The first (POWER_CENTER) is a steep slope used to control 
				how quickly	the power ramps from 0 to some usable level. The second (POWER_SPEED) is a shallower 
				slope used to control how power increases as motor speed increases. The system calculates both 
				curves and then uses the lowest value.
			---------------------------------------------------------------------------------------------------		
			*/			
				#define POWER_CENTER_OFFSET 0 
					/*The is a number between 0 and 10 which control the percent power applied to the motor
					 *when it is set to 0 RPM */
					
				#define POWER_CENTER_INTERCEPT  10
					/* The RPM the motor will be spinning at when the POWER_CENTER line has reached 100% power. */
			
				#define POWER_SPEED_OFFSET  5
					/* The power which should be applied to the motor after it has completed ramping up from 0 rpm
					   (using the POWER_CENTER line). This is a number between 0 and 10 which corresponds to the 
					   percent power.	*/
				#define POWER_SPEED_INTERCEPT  350
				   /* The RPM the motor will be spinning at when we reach full power. */		
	/*
	---------------------------------------------------------------------------------------------------
	SERVO SCALING METHODS
		The following settings controls how we convert between a servo pulsewidth and the 
		speed the motor will run at.
	---------------------------------------------------------------------------------------------------
	*/
			/*
			---------------------------------------------------------------------------------------------------
			 SERVO RANGING 
			---------------------------------------------------------------------------------------------------
			*/	 
					#define SERVO_MIN_US 1000  
						/* The minimum valid servo pulse width in uS. Any value below this number will be ignored */
		
					#define SERVO_MAX_US 2000
						/* The maximum valid servo pulse width in uS. Any value above this number will be ignored */
	
					#define SERVO_CENTER_US 1500
						/* Servo pulse width corresponding to the center point. This value will be interpreted as
						 * zero motor speed. Anything above this number will be positive motor rotation, anything 
						 * below, negative motor rotation. */
		
					#define DEADZONE_US 5
						/* This defines the "deadzone" at the center of the servo range in micro seconds. 
						 * Any servo pulse width which is within (+/-) DEADZONE_US of SERVO_CENTER_US will 
						 * result in zero speed. The speed will then smoothly ramp up from zero upon exiting 
						 * the deadzone.			*/						
			/*
			---------------------------------------------------------------------------------------------------
			 SERVO AVERAGING  
			---------------------------------------------------------------------------------------------------
			*/	 				
					#define AVERAGING_ENABLED
						/* When defined, the average value of the servo pulses, taken over time, will be used
						 * to determine the motor speed. Comment out this definition to disable averaging. */
				
					#define AVERAGING_RATE 7
						/* The averaging intensity indicated by a number between 0 and 10. 0 Corresponds to no 
						   averaging, 10 resulting in full averaging (where the number would never change) */						
			/*
			---------------------------------------------------------------------------------------------------
			 SERVO SCALING 
			---------------------------------------------------------------------------------------------------
			*/	 				
					#define SPEED_SCALE 11
						/* Controls the ratio of servo pulse width uS to RPM where the ratio is
						 *			rpm = (SPEED_SCALE /10)* deltaPulseWidth_uS
						 * where deltaPulseWidth is relative to SERVO_CENTER_US 
						 * A value of 10 gives a 1 to 1 relationship. 20 give 2 to 1, 5 gives a half. */							
			/*
			---------------------------------------------------------------------------------------------------
			 SERVO JITTER FILTERING 
				The jitter filter looks for rouge spikes which last for only one measurement and disregards them.				
			---------------------------------------------------------------------------------------------------
			*/	 	
					#define FILTER_THRESHOLD_US  25		
						/* This is the size of a spike in micro seconds. If the current value is more than 
						   FILTER_THRESHOLD_US from the previous measurement it is considered a spike.	*/
				    #define FILTER_OFF_SLOPE_US 50 
						/* If the previous 2 servo measurements were this far apart, then we are assuming that the
						   servo value is being changed at a fast rate and so we disable the filter. */
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
		/**********************************************************************************************************
		 * PWM_INCREMENT_SCALER
		 *
		 * DESCRIPTION:
		 *  The speed in RPM gets multiplied by this number to determine the number of positions to increment
		 * each PWM cycle 
		 * CALCULATION:
		 *		
		 *
		 *		
		 *		ROT  |  1 MIN       |  1 SEC					    |COIL_RATIO COILS | 255 COUNTS
		 *		_____|______________|_______________________________|_________________|______________|
		 *		     |              |                               |                 |              |
		 *		MIN  |  60 SECONDS  |   1000*PWM_FREQ_KHZ PWM_CYCLES|1 ROT            | 1 COIL
		 *		
		 *		
		 *		rotorIncrement =	speed_rpm *   255 * COIL_RATIO			  COUNTS 
		 *							_______________________________			___________		
		 *							    60*1000*PWM_FREQ_KHZ				 PWM_CYCLE
		 *						= speed_rpm  * (255*7)/60000 = 1785/60000 = 357/12000 
		 *
		 *  MAXIMUM VALUE DURING CALCULATION: (assume max speed of 2000 RPM)
		 *      2000*357 / 12000 =  714000 / 12000 (714,000 fits into a 32bit integer)
		 *			If we dont precalculate the value with COIL_RATIO then:
		 *	    2000*255*7 / 60*1000*1 = 3570000/60000 which also fits into a 32 but integer
		 *		
		 *  MINIMUM SPEED:
		 *      1 = speed_RPM * 357 /12000; speed_rpm = 12000/357 = 33 RPM = .55 rotations per second.
		 ***********************************************************************************************************/
		#define PWM_INCREMENT_SCALER_NUMERATOR     (255*COIL_RATIO) 
		#define PWM_INCREMENT_SCALER_DENOMENATOR  (60U * 1000U * PWM_FREQ_KHZ)
				/**The speed in RPM gets multiplied by this number to determine the number of positions to increment 
				 * each PWM cycle */
				
									

		#ifdef PWM_SEQUENTIAL
			#define SINE_TOTAL 384
			/* When the each phase is shifted 120 degrees, the sum of the sine wave will total 
			   a constant amount. This consistent sum is defined here for use in calculations in 
			   this class.			*/
		#endif
		
		# define SINE_FULL_SCALE 255
		/* Full scale value of the sine function (implemented in the Cpp File) */
		
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& CLASS DEFINITION
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/	
	
/********************************************************************************************************/
/* CLASS: bldcGimbal																					*/
/** 3 Channel Pulse Width Modulator designed specifically for brushless DC Motors. 
 *																										*/
/********************************************************************************************************/
class bldcGimbal
{			
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
	
			bldcGimbal(void);
			/**<Instatiator. Called automatically when the class is instantiated. Good place for class
			 * property initialization																	*/
			/*------------------------------------------------------------------------------------------*/
		

			void begin(void);
			/**< Setup method for class. Call this after the class is instantiated, but before using 
				* the class.																			*/
			/*------------------------------------------------------------------------------------------*/
			
	
			 void tickle(void);
			/**< This function needs to be called on a regular basis to enable the this class to do
			 * housekeeping. Primarily, this is used to increment the motor at the proper times			 */
			 /*------------------------------------------------------------------------------------------*/
			 
			 
			 bool set_servo_us(int16_t value);
			 /**< This method takes a value read from a servo (in microseconds) and performs the scaling 
			  * and preprocessing to allow this value to control the motor speed. This accepts a range 
			  * from 1000uS to 2000uS where zero is 1500uS, anything greated than 1500uS is positive motor
			  * rotation, and anything less than 1500uS is negative motor rotation.
			  * @param value
			  *		The servo pulse width measured in microseconds.										
			  *	@return 
			  *		True if success, false if failure.							   		     			  */
			 /*-------------------------------------------------------------------------------------------*/

			/*
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			&&& ACCESSORS
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			*/
			 
					inline uint16_t speed_rpm(void){return _speed_rpm;};
						 /**< Accessor Method. See corresponding private property for more info.				*/
					inline bldcPwm motorPwm (void){return _motorPwm;} 
						 /**< Accessor Method. See corresponding private property for more info.				*/
					inline uint8_t powerScale(void) {return _powerScale;}
						 /**< Accessor Method. See corresponding private property for more info.				*/
			/*
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			&&& MUTATORS
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			*/
								
					
						
					bool set_speed_rpm(int16_t value); 
						 /**< Mutator Method. See corresponding private property for more info.					*/
					inline bool set_PowerScale(uint8_t value) 
						/**< Mutator Method. See corresponding private property for more info.					*/
					{
						_powerScale = value;
						if(_powerScale >100) _powerScale = 100;
						return true;
					}
						 

	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE PROPERTIES
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:
	
		bldcPwm _motorPwm;
			/**< object which allows pwm control of the H bridges */
			
		uint16_t _speed_rpm;
			/**< The set speed of the motor in rotations per minute */
			
		int8_t _baseIncrement;
			/**< This is the amount the motor needs to be incremented every PWM cycle */
			
		uint16_t _incrementDelay_100us;  
			/**< The accumulator is incremented by one each time this amount of time passes. 
			 *	1 count is 100 microseconds */
			
			
		uint8_t _accumulator; 
			/**< Accumulates increment requests in between PWM cycles. A RPM speed is broken down into 2 parts
			 * 1) A fixed increment amount which gets incremented every PWM cycle (baseIncrement) and a 
			 * incrementDelay which specifies a time interval to add additional increments to the next cycle.
			 * the accumulator stores the increment requests.											  */
						 		
		uint32_t _incrementTimer;			
			/**< stores the last timer value (_100us) when the accumulator was incremented. Used along
			 * with incrementDelay to determine when to increment the accumulator.						*/
			
		 uint8_t _currentStep;
			/**< Holds the motor current PWM setting for the first coil. This is controls the relative
			 * position within each of the motor's coils. Note that most motors have multiple coil pairs
			 * so a full rotation will require more than one full cycle (2-255) of this property.		*/					
		 uint8_t _powerScale;			
			/**< A number between 0 and 10 which controls the power output going to the motor. 10 is
			 * full power, 0 is no power																*/
		 bool _reverse; //When true the motor goes in reverse, otherwise it goes forward.				*/		
	
	
	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE METHODS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:			
	
	
	
	
		void incrementRotor(uint8_t value);
		/**< Increments the motor by the specied number of positions 
		*	@param value
		*		The number of positions to increment the motor by.									*/
		/*------------------------------------------------------------------------------------------*/
					
		inline uint16_t sineToDutyCycle(uint8_t value)
		/**< Scales an 8 but sine value into a valid duty cycle value.
		 * @param value
		 *     8 bit sine value.
		 * @return
		 *     A value PWM duty cycle value.														*/
		/*------------------------------------------------------------------------------------------*/		 
		{			
			#ifndef PWM_SEQUENTIAL
			
			/*  The Equation is:
				(_powerScale * value * kDutyCycleFullScale) / (kFullPowerScale * kSineFullScale) = 
				(_powerScale * value * 1000) / (100 * 255) = 
				(_powerScale * value * 10) / 255 = 
				(_powerScale * value * 2 * 5) / 255 = 
				(_powerScale * value * 2 ) / 51 				
				Maximum Value During Calc = (_powerScale * value * 2 ) = 100*255*2 = 51000 < 65536 (16 bit unsigned full scale)
			*/							
				return ((uint16_t)_powerScale * (uint16_t)value * 2) /51;
				#if POWER_FULL_SCALE != 100 || SINE_FULL_SCALE != 255 || kDutyCycleFullScale !=1000
					#warning Manual Calculation Must Be Redone - POWER_FULL_SCALE, SINE_FULL_SCALE or kDutyCycleFullScale has changed.
				#endif
				

			#else
				#define GENERIC_SCALER 64U								
			/*  The Equation is:
				(_powerScale * value * kDutyCycleFullScale) / (kFullPowerScale * SINETOTAL) = 
				(_powerScale * value * 1000) / (100 * 384) = 
				(_powerScale * value) * 1000 /38400 =
				_powerScale * value * 2 * 5/ 384 = 
				((_powerScale * value / 2) * 5) / 96				
				
				Maximum Value During Calc = (_powerScale * value / 2) * 5 = 63750 < 65536
			*/			
			
			return ((((uint16_t) _powerScale * value*(uint16_t) / 2) * 5) /96)			
			#if POWER_FULL_SCALE != 100 || SINETOTAL != 384 || kDutyCycleFullScale !=1000
				#warning Manual Calculation Must Be Redone - POWER_FULL_SCALE, SINETOTAL or kDutyCycleFullScale has changed.
			#endif				

				
				
			#endif
		}
				
		void calcPowerScale(uint16_t speed);
		/**< Given a speed (in RPM) calculates the percent power which should be applied (based on values in the 
		 * user configuration). It then sets _powerScale to the proper value.								 
		 * @param speed
		 *    The speed in RPM to use when calculating the power											 */
		/*---------------------------------------------------------------------------------------------------*/
		
					
					
					
					
					
};
#endif