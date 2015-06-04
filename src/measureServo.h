/***************************************************************************************//**
 * @brief C Header File for measureServo class which is a driver for reading the pulse
 *        width of the Timer 1 input compare pin, geared towards servo measurements.
 * @details
 *		This file contains the class definition. It also serves as the primary location
 *		for documenting all of the class methods, properties, and structures. When given 
 *		a choice, comments will be placed in this file, unless they are specific to
 *		code implementation, or reference an item only found in the C file.
 *
 * @author Alan Nise, Oceanlab LLC
 * @
 *//***************************************************************************************/



/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INCLUDES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

#include <inttypes.h>
		
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& CLASS DEFINITION
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/	
	
/********************************************************************************************************/
/* CLASS: measureServo																					*/
/** Servo Pulse Width measurement from Timer1 Input Compare Specifically tailored to operation 
 *	with bldcPwm class.  The bldcPwm uses Timer1 already in CPC mode, which resets the timer 
 *  at each input compare. This prevents us from utilizing the time stamp provided by the 
 *  Timer1 input compare hardware, and forces us to use the less accurate method of 
 *  getting the time stamp from our millis class. 
 *																										*/
/********************************************************************************************************/
class measureServo
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
	
		
		measureServo(void);
		/**<Instatiator. Called automatically when the class is instantiated. Good place for class
			* property initialization																	*/
		/*------------------------------------------------------------------------------------------*/
		
		void begin(void);
		/**< Setup method for class. Call this after the class is instantiated, but before using 
			* the class.																			*/
		/*------------------------------------------------------------------------------------------*/
			
		bool changeDetected(void);
		/**< Used to detect if a new servo value has been received. This  must be 
		 * checked before reading the servo value (using value_uS) as the value 
		 * is not valid unless changeDetected is true.
		 * @return 
		 *		True if a new servo value has been received, false if not.							*/
		/*------------------------------------------------------------------------------------------*/
	
		uint16_t value_uS(void);
		/**< Returns the last measured servo pulse width in micro seconds. You must make sure
		 * that changeDetected is true before calling this method or it will return an invalid 
		 * value.																					*/
		/*------------------------------------------------------------------------------------------*/ 
		 
		 
		 
		/*
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			&&& ACCESSORS
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			*/
			 
					
						 /**< Accessor Method. See corresponding private property for more info.				*/
					
			/*
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			&&& MUTATORS
			&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			*/
			
						/**< Mutator Method. See corresponding private property for more info.					*/
						
/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE PROPERTIES
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:



/*
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	&&& PRIVATE METHODS
	&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	*/	private:						
};
