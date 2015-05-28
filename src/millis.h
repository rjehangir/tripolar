
#ifndef MILLIS_H
#define MILLIS_H

		extern uint32_t _ms;
		inline void millis_init(void)
		{
			TCCR0 = 0x03; //Init Timer0, normal, prescalar = 64 (250Khz Clock Rate)
			TCNT0 = 0x05; //Count up from 5 which makes the timer expire in 250 cycles.
			TIMSK |= 0x01; //Set TOIE bit
		}		
		inline uint32_t millis(void){return _ms;}		
#endif