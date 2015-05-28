#include <avr/interrupt.h>
#include "millis.h"

uint32_t _ms;

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 0x05; 
	_ms++;
}

