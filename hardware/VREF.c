#include "VREF.h"

uint8_t adcReference = CONFIG_ADC_REF;
uint8_t dacReference = CONFIG_DAC_REF;

void VREF_init(void) {
    // Enable output for OC2A and OC2B (PD7 and PD6)
    DDRD |= _BV(7) | _BV(6);

    TCCR2A = _BV(WGM20) |
             _BV(WGM21) |
             _BV(COM2A1)|
             _BV(COM2B1);   

    TCCR2B = _BV(CS20);

    OCR2A = adcReference;
    OCR2B = dacReference;
}


void vref_setADC(uint8_t value) {
	adcReference = value;
	OCR2A = adcReference;
}

void vref_setDAC(uint8_t value) {
	dacReference = value;
	OCR2B = dacReference;
}