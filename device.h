#include "util/constants.h"

#ifndef DEVICE_CONFIGURATION
#define DEVICE_CONFIGURATION

// Version info
#define MAJ_VERSION  0x01
#define MIN_VERSION  0x03

// CPU settings
#define TARGET_CPU m1284p
#define F_CPU 20000000UL
#define FREQUENCY_CORRECTION 0

// Voltage references
#define CONFIG_ADC_REF 128
#define CONFIG_DAC_REF 255

#define CONFIG_LED_INTENSITY 192
#define CONFIG_COM_LED_TIMEOUT_MS 40
#define CONFIG_LED_UPDATE_INTERVAL_MS 40

// Demodulator settings
#define OPEN_SQUELCH true

// Serial settings
#define SERIAL_DEBUG false
#define TX_MAXWAIT 2UL

// Queue settings. Don't be fooled by free
// memory indications while compiling! With
// dynamic allocations by SD, exFAT and AES,
// these are more or less the hard limit in
// the current configuration.
#define CONFIG_QUEUE_SIZE 5000
#define CONFIG_QUEUE_MAX_LENGTH 15
#define CONFIG_UART0_BUFFER_SIZE 1536
#define CONFIG_UART1_BUFFER_SIZE 128
#define CONFIG_SERIAL_TIMEOUT_MS 10
#define CONFIG_BENCHMARK_MODE false

// CSMA Settings
#define CONFIG_FULL_DUPLEX false
#define CONFIG_CSMA_P_DEFAULT 255
#define CONFIG_CSMA_SLOTTIME_DEFAULT 20

#define AX25_MIN_FRAME_LEN 4
#define AX25_MAX_FRAME_LEN 611
#define AX25_MAX_PAYLOAD   576
#define AX25_MIN_PAYLOAD   2
#define AX25_ENCRYPTED_MIN_LENGTH 51
// The minimum packet length of an AES-128
// encrypted packet is equal to:
// padding byte + IV + 1 Block + HMAC + CRC

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

// File system paths
#define PATH_BASE "OpenModem"
#define PATH_LOG "OpenModem/Log"
#define PATH_LOG_INDEX "OpenModem/Log/packet.index"

#define PATH_ENTROPY_INDEX "OpenModem/entropy.index"
#define PATH_ENTROPY_SOURCE "OpenModem/entropy.source"
#define PATH_AES_128_KEY "OpenModem/aes128.key"
#define PATH_CRYPTO_DISABLE "OpenModem/aes128.disable"

#endif