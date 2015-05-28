#include <avr/interrupt.h>
#include "millis.h"

uint32_t _100us;

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 59; 
	_100us++;	
}

