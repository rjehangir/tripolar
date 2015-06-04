/***************************************************************************************//**
 * @brief C implementation file for bldcGimbal class.	
 * @details
 *		The documentation strategy is to document the header file as much as possible
 *		and only comment this CPP file for things not already documented in the header
 *	    file.  
 *		
 *		See bldcGimbal.h for an in-depth description of this class, its methods,
 *		and its properties.
 * @author Alan Nise, Oceanlab LLC
 * @
 *//***************************************************************************************/


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INCLUDES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	#include "millis.h"
	#include "bldcGimbal.h"
	#include <stdlib.h>
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& MACROS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	#define PHASE_SHIFT_DEGREES 120  //The sine wave phase shift between each coil in degrees

	#define PHASE_SHIFT  (PHASE_SHIFT_DEGREES * 255)/360
		/* The phase shift converted to the number of pwmSin array elements to displace.
		 *  = PHASE_SHIFT_DEGREES * 255 / 360													*/
		 
    #define SINE_ARRAY_SIZE 256
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& CONSTANTS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	
#ifndef PWM_SEQUENTIAL
	/*****************************************************************************************************
	 * ARRAY: pwmSin
	 * DESCRIPTION:
	 * This is an implementation of a state space sine wave function. This was calculated using 
	 * the spreadsheet misc/calcs/BLDC_SPWM_Lookup_tables.ods based on the original spreadsheet found
	 * at http://www.berryjam.eu/wp-content/uploads/2015/04/BLDC_SPWM_Lookup_tables.ods
	 * This the PWM output corresponds to an element of this array phase shifted 85 degrees. Each
	 * element is 360/255 degrees = 1.4117 degrees. 
	*****************************************************************************************************/			
	const uint8_t pwmSin[SINE_ARRAY_SIZE]=
	{		
				128, 	131, 	134, 	137, 	140, 	143, 	146, 	149, 	152, 	156, 	//	0	 to 	9
				159, 	162, 	165, 	168, 	171, 	174, 	176, 	179, 	182, 	185, 	//	10	 to 	19
				188, 	191, 	193, 	196, 	199, 	201, 	204, 	206, 	209, 	211, 	//	20	 to 	29
				213, 	216, 	218, 	220, 	222, 	224, 	226, 	228, 	230, 	232, 	//	30	 to 	39
				234, 	236, 	237, 	239, 	240, 	242, 	243, 	245, 	246, 	247, 	//	40	 to 	49
				248, 	249, 	250, 	251, 	252, 	252, 	253, 	254, 	254, 	254, 	//	50	 to 	59
				254, 	254, 	254, 	254, 	254, 	254, 	254, 	254, 	254, 	254, 	//	60	 to 	69
				254, 	254, 	253, 	252, 	252, 	251, 	250, 	249, 	248, 	247, 	//	70	 to 	79
				246, 	245, 	243, 	242, 	240, 	239, 	237, 	236, 	234, 	232, 	//	80	 to 	89
				230, 	228, 	226, 	224, 	222, 	220, 	218, 	216, 	213, 	211, 	//	90	 to 	99
				209, 	206, 	204, 	201, 	199, 	196, 	193, 	191, 	188, 	185, 	//	100	 to 	109
				182, 	179, 	176, 	174, 	171, 	168, 	165, 	162, 	159, 	156, 	//	110	 to 	119
				152, 	149, 	146, 	143, 	140, 	137, 	134, 	131, 	128, 	124, 	//	120	 to 	129
				121, 	118, 	115, 	112, 	109, 	106, 	103, 	99, 	96, 	93, 	//	130	 to 	139
				90, 	87, 	84, 	81, 	79, 	76, 	73, 	70, 	67, 	64, 	//	140	 to 	149
				62, 	59, 	56, 	54, 	51, 	49, 	46, 	44, 	42, 	39, 	//	150	 to 	159
				37, 	35, 	33, 	31, 	29, 	27, 	25, 	23, 	21, 	19, 	//	160	 to 	169
				18, 	16, 	15, 	13, 	12, 	10, 	9, 		8,		7,		6, 		//	170	 to 	179
				5,		4,		3,		3,		2,		1,		1,		1, 		1,		1, 		//	180	 to 	189
				1, 		1,	 	1,		1,	 	1,	 	1,	 	1,		1,	 	1,		1, 		//	190	 to 	199
				2, 		3,		3,		4,	 	5,		6,		7,		8,		9,		10, 	//	200	 to 	209
				12, 	13, 	15, 	16, 	18, 	19, 	21, 	23, 	25, 	27, 	//	210	 to 	219
				29, 	31, 	33, 	35, 	37, 	39, 	42, 	44, 	46, 	49, 	//	220	 to 	229
				51, 	54, 	56, 	59, 	62, 	64, 	67, 	70, 	73, 	76, 	//	230	 to 	239
				79, 	81, 	84, 	87, 	90, 	93, 	96, 	99, 	103, 	106, 	//	240	 to 	249
				109, 	112, 	115, 	118, 	121, 	124										//	250	 to 	255
	};
		
#else		
		/*****************************************************************************************************
		 * ARRAY: pwmSin
		 * DESCRIPTION:
		 * This is an implementation of a standard sine wave table
		 * This was calculated using
		 * the spreadsheet misc/calcs/BLDC_SPWM_Lookup_tables.xlsx based on the original spreadsheet found
		 * at http://www.berryjam.eu/wp-content/uploads/2015/04/BLDC_SPWM_Lookup_tables.ods
		*****************************************************************************************************/			
		const uint8_t pwmSin[SINE_ARRAY_SIZE]=
		{			
			128,	131,	134,	137,	140,	143,	146,	149,	152,	155,	// From 0 To 9
			158,	162,	165,	167,	170,	173,	176,	179,	182,	185,	// From 10 To 19
			188,	190,	193,	196,	198,	201,	203,	206,	208,	211,	// From 20 To 29
			213,	215,	218,	220,	222,	224,	226,	228,	230,	232,	// From 30 To 39
			234,	235,	237,	238,	240,	241,	243,	244,	245,	246,	// From 40 To 49
			248,	249,	250,	250,	251,	252,	253,	253,	254,	254,	// From 50 To 59
			254,	255,	255,	255,	255,	255,	255,	255,	254,	254,	// From 60 To 69
			254,	253,	253,	252,	251,	250,	250,	249,	248,	246,	// From 70 To 79
			245,	244,	243,	241,	240,	238,	237,	235,	234,	232,	// From 80 To 89
			230,	228,	226,	224,	222,	220,	218,	215,	213,	211,	// From 90 To 99
			208,	206,	203,	201,	198,	196,	193,	190,	188,	185,	// From 100 To 109
			182,	179,	176,	173,	170,	167,	165,	162,	158,	155,	// From 110 To 119
			152,	149,	146,	143,	140,	137,	134,	131,	128,	124,	// From 120 To 129
			121,	118,	115,	112,	109,	106,	103,	100,	97,		93,		// From 130 To 139
			90,		88,		85,		82,		79,		76,		73,		70,		67,		65,		// From 140 To 149
			62,		59,		57,		54,		52,		49,		47,		44,		42,		40,		// From 150 To 159
			37,		35,		33,		31,		29,		27,		25,		23,		21,		20,		// From 160 To 169
			18,		17,		15,		14,		12,		11,		10,		9,		7,		6,		// From 170 To 179
			5,		5,		4,		3,		2,		2,		1,		1,		1,		0,		// From 180 To 189
			0,		0,		0,		0,		0,		0,		1,		1,		1,		2,		// From 190 To 199
			2,		3,		4,		5,		5,		6,		7,		9,		10,		11,		// From 200 To 209
			12,		14,		15,		17,		18,		20,		21,		23,		25,		27,		// From 210 To 219
			29,		31,		33,		35,		37,		40,		42,		44,		47,		49,		// From 220 To 229
			52,		54,		57,		59,		62,		65,		67,		70,		73,		76,		// From 230 To 239
			79,		82,		85,		88,		90,		93,		97,		100,	103,	106,	// From 240 To 249
			109,	112,	115,	118,	121,	124 									// From 250 To 255
		};
		
		
																
			   
#endif
	
	
					
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& CLASS METHOD IMPLEMENTATION FUNCTIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

	 	 
	/****************************************************************************
	*  Class: bldcGimbal
	*  Method: bldcGimbal
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	bldcGimbal::bldcGimbal(void)
	{					
		 _baseIncrement = 0;			
		 _incrementDelay_100us = 1000;  			
		 _accumulator = 0; 									 		
		 _incrementTimer = 0;									
		 _currentStep = 0;	
		 _powerScale = 4;				
	}
			
	/****************************************************************************
	*  Class: bldcGimbal
	*  Method: bldcGimbal
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/	
	void bldcGimbal::begin(void)
	{
		_motorPwm.begin();
	}
			
	/****************************************************************************
	*  Class: bldcGimbal
	*  Method: bldcGimbal
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/		
	void bldcGimbal::tickle(void)
	{												
			_motorPwm.tickle();
			uint16_t timerVal = _100micros();
			
			if (_incrementDelay_100us == 0) _accumulator = 0;
			else
			{
				if (timerVal - _incrementTimer >= _incrementDelay_100us){
					_incrementTimer = timerVal;
					_accumulator++;
				}				
			}
			
						
			if (!_motorPwm.busy()){
				incrementRotor(_baseIncrement + _accumulator);
				_accumulator = 0;
			}					
	}
	
	/****************************************************************************
	*  Class: bldcGimbal
	*  Method: bldcGimbal
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/				 
	void bldcGimbal::set_speed_rpm(int16_t value)
	{						
		if(value ==0)
		{
			_incrementDelay_100us = 0;
			_baseIncrement = 0;
			return;
		}
		
		if (value < 0) _reverse = true;
		else _reverse = false;
		_speed_rpm = value;
		uint16_t speed = abs(value);
		int32_t numerator = PWM_INCREMENT_SCALER_NUMERATOR;
		numerator *= speed;
		numerator *= 1000;
		uint32_t denomenator = 	PWM_INCREMENT_SCALER_DENOMENATOR;
		uint32_t calcValue = numerator / denomenator;
		_baseIncrement = calcValue / 1000;
		uint32_t remainder = calcValue - ((uint32_t)_baseIncrement *1000);
		_incrementDelay_100us = 10000 / remainder;				
	}
	

	/****************************************************************************
	*  Class: bldcGimbal
	*  Method: bldcGimbal
	*	Description:
	*		See class header file for a full API description of this method
	****************************************************************************/			
	void bldcGimbal::incrementRotor(uint8_t value)
	{
				
		uint16_t pwmA,pwmB,pwmC;
		uint8_t indexA,indexB,indexC;
		
		if (_reverse) _currentStep -= value;
		else _currentStep += value;
	
		indexA = _currentStep;
		indexB = _currentStep + PHASE_SHIFT;
		indexC = indexB + PHASE_SHIFT;
		
		pwmA = sineToDutyCycle(pwmSin[indexA]);
		pwmB = sineToDutyCycle(pwmSin[indexB]);
		pwmC = sineToDutyCycle(pwmSin[indexC]);
			
		_motorPwm.set_pwm(bldcPwm::ePwmChannel_A,pwmA);
		_motorPwm.set_pwm(bldcPwm::ePwmChannel_B,pwmB);
		_motorPwm.set_pwm(bldcPwm::ePwmChannel_C,pwmC);
		_motorPwm.update();
	};	