#ifndef _PROTOCOL_KISS
#define _PROTOCOL_KISS 0x02

#include "../hardware/AFSK.h"
#include "../hardware/Serial.h"
#include "../util/time.h"
#include "AX25.h"

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#define CMD_UNKNOWN 0xFE
#define CMD_DATA 0x00
#define CMD_PREAMBLE 0x01
#define CMD_P 0x02
#define CMD_SLOTTIME 0x03
#define CMD_TXTAIL 0x04
#define CMD_FULLDUPLEX 0x05
#define CMD_SETHARDWARE 0x06
#define CMD_SAVE_CONFIG 0x07
#define CMD_LED_INTENSITY 0x08
#define CMD_OUTPUT_GAIN 0x09
#define CMD_INPUT_GAIN 0x0A
#define CMD_PASSALL 0x0B
#define CMD_LOG_PACKETS 0x0C
#define CMD_GPS_MODE 0x0D
#define CMD_BT_MODE	0x0E
#define CMD_READY 0x0F
#define CMD_SERIAL_BAUDRATE 0x10
#define CMD_REBOOT 0x11
#define CMD_REBOOT_CONFIRM 0x9A
#define CMD_AUDIO_PEAK 0x12
#define CMD_ENABLE_DIAGNOSTICS 0x13
#define CMD_MODE 0x14
#define CMD_NMEA 0x40
#define CMD_PRINT_CONFIG 0xF0
#define CMD_RETURN 0xFF
#define CMD_INVERT_SDDETECT 0x15

void kiss_init(AX25Ctx *ax25, Afsk *afsk, Serial *ser);
void kiss_messageCallback(AX25Ctx *ctx);
void kiss_serialCallback(uint8_t sbyte);
void kiss_flushQueue(void);
void kiss_csma(void);
void kiss_poll(void);

void kiss_output_modem_mode(void);
void kiss_output_afsk_peak(void);
void kiss_output_config(uint8_t* data, size_t length);
void kiss_output_nmea(char* data, size_t length);

bool log_init(void);
bool load_log_index(void);
bool update_log_index(void);

#endif