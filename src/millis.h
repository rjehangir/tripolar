
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
			/*
			TCCR2 |= 
					_BV(WGM21) | ~_BV(WGM20) |     //CTC mode (reset timer after expire
				    ~_BV(COM21) | ~_BV(COM20)  | //No Output Compare Match Pin
					~_BV(CS22) | _BV(CS21) | ~_BV(CS20);  //Clock Prescale /8 (2Mhz)*/
			
			TCCR2 = 0;
			TCCR2 |= _BV(WGM21);
			TCCR2 |= _BV(CS21);
			
			
			
			TCNT2 = 0;			
			OCR2 = 199; //Expire after 100uS
			ASSR = 0; //Dont use asych clock, use i/o clock.
			TIMSK |= _BV(OCIE2); //Set Output compare interrupt
											
			timer32_ms = 0;
			timer16_us = 0;
			timerCycles = 0;
			
		}		
		inline uint32_t millis(void)
		{
			TIMSK &= ~_BV(OCIE2); //Set Output compare interrupt		
			uint32_t retVal = timer32_ms;
			TIMSK |= _BV(OCIE2); //Set Output compare interrupt		
			return retVal;
		}		
		inline uint16_t micros(void)
		{
			
		  // return timer16_us + TCNT2/2;		   		    
			uint16_t timerCount = TCNT2;
			TIMSK &= ~_BV(OCIE2); //disable Output compare interrupt			
			volatile uint16_t usCount = timer16_us;									
			TIMSK |= _BV(OCIE2); //Set Output compare interrupt		   
			return usCount  + (timerCount) /2;						
		}
		
		inline uint16_t _100micros(void)
		{			  
			  TIMSK &= ~_BV(OCIE2); //Set Output compare interrupt
			  volatile uint16_t usCount = timer16_us;
			  TIMSK |= _BV(OCIE2); //Set Output compare interrupt
			  return usCount/100;
		}
#endif