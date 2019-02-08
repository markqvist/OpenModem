#include "VREF.h"
#include "util/Config.h"

void VREF_init(void) {
    // Enable output for OC2A and OC2B (PD7 and PD6)
    VREF_DDR |= _BV(7) | _BV(6);

    TCCR2A = _BV(WGM20) |
             _BV(WGM21) |
             _BV(COM2A1)|
             _BV(COM2B1);   

    TCCR2B = _BV(CS20);

    OCR2A = config_input_gain;
    OCR2B = config_output_gain;
}


void vref_setADC(uint8_t value) {
	config_input_gain = value;
	OCR2A = config_input_gain;
}

void vref_setDAC(uint8_t value) {
	config_output_gain = value;
	OCR2B = config_output_gain;
}