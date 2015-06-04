//*****************************************
//* AfroESC 3                             *
//* 2012-12-02                            *
//* Fuses should be lfuse=0x3f hfuse=0xca *
//*****************************************

#include <avr/io.h>
#include <avr/interrupt.h>

//#define	F_CPU		= 16000000
#define	USE_INT0	= 0
#define	USE_I2C		= 1
#define	USE_UART	= 1
#define	USE_ICP		= 1

#define	DEAD_LOW_NS	= 300
#define	DEAD_HIGH_NS	= 300
#define	MOTOR_ADVANCE	= 15
#define	CHECK_HARDWARE	= 1

//;*********************
//; PORT B definitions *
//;*********************

#define CnFET		3
#define	red_led		1	//; o
#define	green_led	2	//; o
#define rcp_in  0	//i r/c pulse input

#define	INIT_PB	(1<<CpFET)
#define DIR_PB  (1<<CpFET)

#define	CnFET_port PORTB

//;*********************
//; PORT C definitions *
//;*********************
#define	mux_voltage	7	//; ADC7 voltage input (18k from Vbat, 3.3k to gnd, 10.10V -> 1.565V at ADC7)
#define	mux_temperature	6	//; ADC6 temperature input (3.3k from +5V, 10k NTC to gnd)
#define	i2c_clk		5	//; ADC5/SCL
#define	i2c_data	4	//; ADC4/SDA

#define	mux_b		  1	//; ADC1 phase input
#define	mux_a		  0	//; ADC0 phase input

#define	O_POWER		180
#define	O_GROUND	33

#define	INIT_PC		(1<<i2c_clk)+(1<<i2c_data)

#define DIR_PC (1<<green_led) + (1<<red_led)
inline void DEBUG_OUT(uint8_t X) {}


inline void redOn(){PORTB &= ~_BV(red_led);}
inline void redOff(){PORTB |= _BV(red_led);}
inline void greenOn(){PORTB &= ~_BV(green_led);}
inline void greenOff(){PORTB |= _BV(green_led);}

//;*********************
//; PORT D definitions *
//;*********************

#define	CpFET		7
#define	CENTER		6
#define	BnFET		5
#define	BpFET		4
#define	AnFET		3
#define	ApFET		2
#define	txd		  1
#define	rxd		  0



#ifndef DO_DEBUG
	#define	DIR_PD		(1<<AnFET)+(1<<BnFET)+(1<<CnFET)+(1<<ApFET)+(1<<txd)
	#define	INIT_PD		(1<<ApFET)+(1<<txd)
#else
	#define	DIR_PD		(1<<ApFET) + (1<<AnFET)+(1<<BpFET)+(1<<BnFET)+(1<<CpFET)
	#define	INIT_PD		(1<<ApFET)+(1<<txd)
#endif

#define	ApFET_port	PORTD
#define	AnFET_port	PORTD
#define	BpFET_port	PORTD
#define	BnFET_port	PORTD
#define	CpFET_port	PORTD


inline void ApFETOn()  { ApFET_port &= ~_BV(ApFET);}
inline void ApFETOff() { ApFET_port |=  _BV(ApFET);}
inline void AnFETOn()  { AnFET_port |=  _BV(AnFET);}
inline void AnFETOff() { AnFET_port &= ~_BV(AnFET);}

inline void BpFETOn()  { BpFET_port &= ~_BV(BpFET);}
inline void BpFETOff() { BpFET_port |=  _BV(BpFET);}
inline void BnFETOn()  { BnFET_port |=  _BV(BnFET);}
inline void BnFETOff() { BnFET_port &= ~_BV(BnFET);}

inline void CpFETOn()  { CpFET_port &= ~_BV(CpFET);}
inline void CpFETOff() { CpFET_port |=  _BV(CpFET);}
inline void CnFETOn()  { CnFET_port |=  _BV(CnFET);}
inline void CnFETOff() { CnFET_port &= ~_BV(CnFET);}

inline void boardInit() {
	//TIMSK1 = 0;
	//TCCR1A = 0;
	//TCCR1B = _BV(CS11);
	PORTB = INIT_PB;
	PORTC = INIT_PC;
	PORTD = INIT_PD;
	DDRB = DIR_PB;
	DDRC = DIR_PC;
	DDRD = DIR_PD;
}
