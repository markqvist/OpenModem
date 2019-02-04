#include "util/constants.h"

#ifndef DEVICE_CONFIGURATION
#define DEVICE_CONFIGURATION

// CPU settings
#define TARGET_CPU m1284p
#define F_CPU 20000000UL
#define FREQUENCY_CORRECTION 0

// Voltage references
// TODO: Determine best defaults
#define CONFIG_ADC_REF 255
#define CONFIG_DAC_REF 128

// TODO: Change this back to default
#define CONFIG_LED_INTENSITY 35
//#define CONFIG_LED_INTENSITY 192
#define CONFIG_COM_LED_TIMEOUT_MS 40
#define CONFIG_LED_UPDATE_INTERVAL_MS 40

// Demodulator settings
#define OPEN_SQUELCH true

// Serial settings
#define SERIAL_DEBUG false
#define TX_MAXWAIT 2UL
#define CONFIG_QUEUE_SIZE 6000 // TODO: Optimise this by saving ram other places, add SD queue
#define CONFIG_QUEUE_MAX_LENGTH 15
#define CONFIG_UART0_BUFFER_SIZE 1536 // TODO: Tune this, what is actually required?
#define CONFIG_UART1_BUFFER_SIZE 128
#define CONFIG_SERIAL_TIMEOUT_MS 10
#define CONFIG_BENCHMARK_MODE false

// CSMA Settings
#define CONFIG_FULL_DUPLEX false  // TODO: Actually implement fdx
#define CONFIG_CSMA_P 255

#define AX25_MIN_FRAME_LEN 1
#define AX25_MAX_FRAME_LEN 600

// Packet settings
#define CONFIG_PASSALL false

// Port settings
#if TARGET_CPU == m1284p
    #define ADC_PORT  	PORTA
    #define ADC_DDR   	DDRA

    #define DAC_PORT  	PORTC
    #define DAC_DDR   	DDRC

	#define VREF_PORT 	PORTD
	#define VREF_DDR  	DDRD

    #define LED_PORT  	PORTB
    #define LED_DDR   	DDRB

	#define PTT_DDR   	DDRD
	#define PTT_PORT  	PORTD
	#define PTT_PIN   	5
	#define PTT_NEG_PIN	4

	#define SPI_PORT    PORTB
	#define SPI_DDR		DDRB
	#define SPI_MOSI	5
	#define SPI_MISO	6
	#define SPI_CLK		7

	#define SD_CS_DDR	DDRA
	#define SD_CS_PORT  PORTA
	#define SD_CS_PIN   6
	#define SD_DETECT_DDR	DDRA
	#define SD_DETECT_PORT  PORTA
	#define SD_DETECT_INPUT PINA
	#define SD_DETECT_PIN	7

	#define BT_DDR		DDRA
	#define BT_PORT		PORTA
	#define BT_INPUT	PINA
	#define BT_MODE		3
	#define BT_RTS		4

	#define GPS_DDR		DDRA
	#define GPS_PORT	PORTA
	#define GPS_INPUT	PINA
	#define GPS_EN_PIN	5

	#define USR_IO_DDR	DDRA
	#define USR_IO_PORT	PORTA
	#define USR_IO_1	1
	#define USR_IO_2	2
	#define USR_IO_3	3
	#define USR_IO_4	4
#endif

#endif

/*
PA0		ANALOG_IN
PA1		USR_1
PA2		USR_2
PA3		USR_3 / BT_MODE		// TODO: Set as output
PA4		USR_4 / BT_RTS		// TODO: Set as input
PA5		GPS_EN				// TODO: Set as output/input
PA6		SD_CS				// TODO: Set as output
PA7		SD_DETECT			// TODO: Set as input and enable pullup

PB0		LED_RX
PB1		LED_TX
PB2		LED_STATUS
PB3		LED_DRAIN_PWM
PB4		LED_COM / SPI_SS	(PGM)
PB5		SPI_MOSI			SD/PGM
PB6		SPI_MISO			SD/PGM
PB7		SPI_CLK				SD/PGM

PC0		DAC_0
PC1		DAC_1
PC2		DAC_2
PC3		DAC_3
PC4		DAC_4
PC5		DAC_5
PC6		DAC_6
PC7		DAC_7

PD0		UART0_RX
PD1		UART0_TX
PD2		UART1_RX		GPS
PD3		UART1_TX		GPS
PD4		PTT_NEG
PD5		PTT_SIG
PD6		REF_DAC
PD7		REF_ADC
*/