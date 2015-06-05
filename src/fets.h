#ifndef FETS_H
#define FETS_H



// UNCOMMENT ONE OF THESE FILES ONLY DEPENDING ON WHICH ESC YOU ARE USING
//#include "afro_nfet.h"
#include "blue_nfet.h"

inline void highSideOff() {
	ApFETOff();
	CpFETOff();
	BpFETOff();
}

inline void lowSideOff() {
	AnFETOff();
	BnFETOff();
	CnFETOff();
}

inline void commStateHighSideOn(uint8_t state) {
	switch(state) {
		case 0:
			ApFETOn();
			BpFETOff();
			CpFETOff();
			break;
		case 1:
			ApFETOff();
			BpFETOff();
			CpFETOn();
			break;
		case 2:
			ApFETOff();
			BpFETOff();
			CpFETOn();
			break;
		case 3:
			ApFETOff();
			BpFETOn();
			CpFETOff();
			break;
		case 4:
			ApFETOff();
			BpFETOn();
			CpFETOff();
			break;
		case 5:
			ApFETOn();
			BpFETOff();
			CpFETOff();
			break;
	}
}

inline void commStateLowSideOn(uint8_t state) {
	switch(state) {
		case 0:
			AnFETOff();
			BnFETOn();
			CnFETOff();
			break;
		case 1:
			AnFETOff();
			BnFETOn();
			CnFETOff();
			break;
		case 2:
			AnFETOn();
			BnFETOff();
			CnFETOff();
			break;
		case 3:
			AnFETOn();
			BnFETOff();
			CnFETOff();
			break;
		case 4:
			AnFETOff();
			BnFETOff();
			CnFETOn();
			break;
		case 5:
			AnFETOff();
			BnFETOff();
			CnFETOn();
			break;
	}
}

#endif