/* Tripolar Stepper Motor Driver
-------------------------------
 
Title: Atmega8 Tripolar Stepper Motor Driver
Description: See README.md

-------------------------------
The MIT License (MIT)

Copyright (c) 2014 by Rustom Jehangir, Blue Robotics Inc.
                      Author #2, Company #2
                      Author #3, Company #3

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-------------------------------*/

/*#include <avr/io.h>
#include <avr/interrupt.h>
#include "afro_nfet.h"
#include "fets.h"

struct {
	volatile uint8_t duty = 50;
	volatile bool on = 0;
} state;


ISR(TIMER1_COMPA_vect) {
	//state.on = !state.on;
	if (TCNT1<state.duty) {
		fetsOff();
	} else {
		commStateOn(2);
	}
}*/

void setup() {
	// Set PORTB pins as output, but off
	//DDRB = 0xFF;
	//PORTB = 0x00;
	
	cli();

	boardInit();
 
	// // Reset timer
	// TCCR1A = 0;
	// TCCR1B = 0;
	// TIMSK1 = 0;
	// TCNT1 = 0;
	// // Set compare match register to overflow immediately
	// OCR1A = 100;
	// // Turn on CTC mode for timer 1
	// TCCR1B |= _BV(WGM12);
	// // Set for no prescaler
	// TCCR1B |= _BV(CS10);
	// // Enable timer compare interrupt
	// TIMSK1 |= _BV(OCIE1A);
	// // Clear any pending interrupts
	// TIFR1 |= _BV(OCF1A); 
	
	sei();
	delay(1000);
}

uint8_t phase;

uint16_t onTime = 10000;

uint8_t dutyCycle = 50;

uint16_t cyclesPerState = 500;

void loop() {
	commStateLowSideOn(phase);
	for ( uint16_t i = 0 ; i < cyclesPerState ; i++ ) {
		commStateHighSideOn(phase);
		delayMicroseconds(max(dutyCycle,97));
		highSideOff();
		delayMicroseconds(min(100-dutyCycle,3));
	}
	phase++;
	if ( phase > 5 ) {
		phase = 0;
	}
}

