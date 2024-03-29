#include <avr/interrupt.h>
#include "millis.h"

uint32_t timer32_ms;
uint16_t timer16_us;
uint8_t timerCycles;

ISR(TIMER2_COMP_vect)
{	
	timer16_us += 100;		
	if (++timerCycles >= 10){
		timerCycles = 0;
		timer32_ms++;
	} 
}

