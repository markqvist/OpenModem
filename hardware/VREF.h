#ifndef VREF_H
#define VREF_H

#include <avr/io.h>
#include "device.h"

void VREF_init(void);
void vref_setADC(uint8_t value);
void vref_setDAC(uint8_t value);

#endif