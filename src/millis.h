
#ifndef MILLIS_H
#define MILLIS_H


/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& INCLUDES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/
	#include <avr/io.h>
		

/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& GLOBAL VARIABLE DECLARATIONS
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/		
	extern uint32_t timer32_ms;
	extern uint16_t timer16_us;
	extern uint8_t timerCycles;

	
		
	
/*
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
&&& FUNCTION PROTOTYPES
&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/	
		inline void millis_init(void)
		{
			TCCR0 = 0x02; //Init Timer0, normal, prescalar = 64 (250Khz Clock Rate)
			TCNT0 = 59; //Count up from 55 which makes the timer expire in 200 cycles.
			TIMSK |= _BV(TOIE0); //Set TOIE bit
			
			timer32_ms = 0;
			timer16_us = 0;
			timerCycles = 0;
			
		}		
		inline uint32_t millis(void){return timer32_ms;}		
		inline uint16_t micros(void)
		{
		    
			return timer16_us  + (TCNT0-59) /2;			
		}
#endif