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
				 * make on rotation																				*/
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
					void set_speed_rpm(int16_t value); 
						 /**< Mutator Method. See corresponding private property for more info.					*/
					inline bool set_PowerScale(uint8_t value) 
						/**< Mutator Method. See corresponding private property for more info.					*/
					{
						_powerScale = value;
						if(_powerScale >4) _powerScale = 4;
						return true;
					}
						 

	/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE METHODS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:			
	
		void incrementRotor(uint8_t value);
		/**< Increments the motor by the specied number of positions 
		*	@param value
		*		The number of positions to increment the motor by.									 */
		/*------------------------------------------------------------------------------------------*/
					
		inline uint16_t sineToDutyCycle(uint8_t value)
		{
			#ifndef PWM_SEQUENTIAL
				#define GENERIC_SCALER 64U
				return ((uint16_t) _powerScale *( uint16_t)value * (((kDutyCycleFullScale/4) * GENERIC_SCALER) / 255))/ (GENERIC_SCALER);
			#else
				#define GENERIC_SCALER 64U
				
				return ((uint16_t) _powerScale * value*(uint16_t)(((kDutyCycleFullScale/4) * GENERIC_SCALER) / SINE_TOTAL))/(GENERIC_SCALER);
					/* THE EQUATION IS  
								value*(255/SINE_TOTAL)*(1023/255).
								= value * (1023/SINE_TOTAL)
					 But this requires either 
				   floating point, or 32 but integer math.  The equation below plays some
				   tricks to have the precompiler do most of the work, and also to 
				   limit the work to 1 16bit multiple and 1 16 bit divide.  */				
			#endif
		}
					
					
					
					
					
};
#endif