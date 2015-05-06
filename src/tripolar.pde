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

#include <avr/io.h>
#include <avr/interrupt.h>
#include "afro_nfet.h"
#include "fets.h"

struct {
	volatile bool on = 0;
	volatile bool state = 0;
	volatile uint8_t pulseWidthA = 0;
	volatile uint8_t pulseWidthB = 0;
	volatile uint8_t pulseWidthC = 0;
} state;

const int pwmSin[] = {128, 132, 136, 140, 143, 147, 151, 155, 159, 162, 166, 170, 174, 178, 181, 185, 189, 192, 196, 200, 203, 207, 211, 214, 218, 221, 225, 228, 232, 235, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 235, 232, 228, 225, 221, 218, 214, 211, 207, 203, 200, 196, 192, 189, 185, 181, 178, 174, 170, 166, 162, 159, 155, 151, 147, 143, 140, 136, 132, 128, 124, 120, 116, 113, 109, 105, 101, 97, 94, 90, 86, 82, 78, 75, 71, 67, 64, 60, 56, 53, 49, 45, 42, 38, 35, 31, 28, 24, 21, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 24, 28, 31, 35, 38, 42, 45, 49, 53, 56, 60, 64, 67, 71, 75, 78, 82, 86, 90, 94, 97, 101, 105, 109, 113, 116, 120, 124};

ISR(TIMER1_COMPA_vect) {
	if ( TCNT2 > 253 ) {
		// Turn low side back off before turning high side on
		lowSideOff();
	}

	// Turn low side back on after delay
	if ( TCNT2 > state.pulseWidthA+10 ) AnFETOn();
	// Turn high side on until duty cycle expires
	if ( TCNT2 < state.pulseWidthA ) { ApFETOn(); AnFETOff(); }
	else ApFETOff();

	// Turn low side back on after delay
	if ( TCNT2 > state.pulseWidthB+10 ) BnFETOn();
	// Turn high side on until duty cycle expires
	if ( TCNT2 < state.pulseWidthB ) { BpFETOn(); BnFETOff(); }
	else BpFETOff();

	// Turn low side back on after delay
	if ( TCNT2 > state.pulseWidthC+10 ) CnFETOn();
	// Turn high side on until duty cycle expires
	if ( TCNT2 < state.pulseWidthC ) { CpFETOn(); CnFETOff(); }
	else CpFETOff();
}

int16_t currentStepA;
int16_t currentStepB;
int16_t currentStepC;
int16_t sineArraySize;
int8_t increment;
bool forward;

void setup() {	
	cli();

	boardInit();

	// Set up timer two, which we will watch with Timer1 interrupt
	TCCR2 |= _BV(CS21);
	TCCR2 |= _BV(CS20);
	TCCR2 &= ~_BV(CS22);
 
	// Reset timer
	TCCR1A = 0;
	TCCR1B = 0;
	TIMSK = 0;
	TCNT1 = 0;
	// Set compare match register to overflow immediately
	OCR1A = 0;
	// Turn on CTC mode for timer 1
	TCCR1B |= _BV(WGM12);
	// Set for no prescaler
	TCCR1B |= _BV(CS10);
	// Enable timer compare interrupt
	TIMSK |= _BV(OCIE1A);
	// Clear any pending interrupts
	TIFR |= _BV(OCF1A); 

	
	sei();

	sineArraySize = sizeof(pwmSin)/sizeof(int);
	int phaseShift = sineArraySize/3;
	currentStepA = 0;
	currentStepB = currentStepA + phaseShift;
	currentStepC = currentStepB + phaseShift;

	sineArraySize--;

	delay(100);
}

uint8_t phase;

void loop() {
	state.pulseWidthA = (pwmSin[currentStepA]-128)*0.6+128;
	state.pulseWidthB = (pwmSin[currentStepB]-128)*0.6+128;
	state.pulseWidthC = (pwmSin[currentStepC]-128)*0.6+128;

	if ( forward ) increment = 1;
	else increment = -1;

	currentStepA += increment;
	currentStepB += increment;
	currentStepC += increment;

	if (currentStepA>sineArraySize) currentStepA = 0;
	if (currentStepA<0) currentStepA = sineArraySize;

	if (currentStepB>sineArraySize) currentStepB = 0;
	if (currentStepB<0) currentStepB = sineArraySize;

	if (currentStepC>sineArraySize) currentStepC = 0;
	if (currentStepC<0) currentStepC = sineArraySize;

	//delay(0);
}

