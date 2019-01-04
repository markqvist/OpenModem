#include "util/constants.h"

#ifndef DEVICE_CONFIGURATION
#define DEVICE_CONFIGURATION

// CPU settings
#define TARGET_CPU m1284p
#define F_CPU 20000000UL
#define FREQUENCY_CORRECTION 0

// Voltage references
// TODO: Determine best defaults
#define CONFIG_ADC_REF 128
#define CONFIG_DAC_REF 255

// Demodulator settings
#define OPEN_SQUELCH true

// Serial settings
#define BAUD 115200
#define SERIAL_DEBUG false
#define TX_MAXWAIT 2UL

// CSMA Settings
#define CONFIG_CSMA_P 255

// Packet settings
#define CONFIG_PASSALL false

// Port settings
#if TARGET_CPU == m1284p
    #define ADC_PORT PORTA
    #define ADC_DDR  DDRA
    #define DAC_PORT PORTB
    #define DAC_DDR  DDRB
    #define LED_PORT PORTC
    #define LED_DDR  DDRC
#endif

#endif