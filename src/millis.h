
#ifndef MILLIS_H
#define MILLIS_H

		
		extern uint32_t _100us;
		inline void millis_init(void)
		{
			TCCR0 = 0x02; //Init Timer0, normal, prescalar = 64 (250Khz Clock Rate)
			TCNT0 = 59; //Count up from 55 which makes the timer expire in 200 cycles.
			TIMSK |= _BV(TOIE0); //Set TOIE bit
		}		
		inline uint32_t millis(void){return _100us/10;}		
#endif