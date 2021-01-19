#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define CONF_VERSION				0x01

#define ADDR_E_MAJ_VERSION			0x00
#define ADDR_E_MIN_VERSION			0x01
#define ADDR_E_CONF_VERSION			0x02
#define ADDR_E_P					0x03
#define ADDR_E_SLOTTIME				0x04
#define ADDR_E_PREAMBLE				0x05
#define ADDR_E_TAIL					0x06
#define ADDR_E_LED_INTENSITY		0x07
#define ADDR_E_OUTPUT_GAIN			0x08
#define ADDR_E_INPUT_GAIN			0x09
#define ADDR_E_PASSALL				0x0A
#define ADDR_E_LOG_PACKETS			0x0B
#define ADDR_E_CRYPTO_LOCK			0x0C
#define ADDR_E_GPS_MODE				0x0D
#define ADDR_E_BLUETOOTH_MODE		0x0E
#define ADDR_E_SERIAL_BAUDRATE		0x0F
#define ADDR_E_INVERT_SDDETECT		0x10
#define ADDR_E_CHECKSUM				0x11
#define ADDR_E_END					0x21

#define CONFIG_GPS_OFF				0x00
#define CONFIG_GPS_AUTODETECT		0x01
#define CONFIG_GPS_REQUIRED			0x02

#define CONFIG_GPS_NMEA_NONE		0x00
#define CONFIG_GPS_NMEA_RAW			0x01
#define CONFIG_GPS_NMEA_ENCAP		0x02

#define CONFIG_BLUETOOTH_OFF		0x00
#define CONFIG_BLUETOOTH_AUTODETECT	0x01
#define CONFIG_BLUETOOTH_REQUIRED	0x02

#define CONFIG_BAUDRATE_1200		0x01
#define CONFIG_BAUDRATE_2400		0x02
#define CONFIG_BAUDRATE_4800		0x03
#define CONFIG_BAUDRATE_9600		0x04
#define CONFIG_BAUDRATE_14400		0x05
#define CONFIG_BAUDRATE_19200		0x06
#define CONFIG_BAUDRATE_28800		0x07
#define CONFIG_BAUDRATE_38400		0x08
#define CONFIG_BAUDRATE_57600		0x09
#define CONFIG_BAUDRATE_76800		0x0A
#define CONFIG_BAUDRATE_115200		0x0B
#define CONFIG_BAUDRATE_230400		0x0C

#define CONFIG_SOURCE_NONE			0x00
#define CONFIG_SOURCE_DEFAULT		0x01
#define CONFIG_SOURCE_EEPROM		0x02
#define CONFIG_SOURCE_SD			0x03

#define CONF_CHECKSUM_SIZE			16

uint8_t config_source;

uint8_t config_p;
unsigned long config_slottime;
unsigned long config_preamble;
unsigned long config_tail;
uint8_t config_led_intensity;
uint8_t config_output_gain;
uint8_t config_input_gain;
bool config_passall;
bool config_log_packets;
bool config_crypto_lock;
uint8_t config_gps_mode;
uint8_t config_gps_nmea_output;
uint8_t config_bluetooth_mode;
uint8_t config_serial_baudrate;
bool config_invert_sddetect;

bool config_user_jobs_enabled;
bool config_output_diagnostics;

bool config_sensors_enabled;
int32_t config_sensor_interval_ms;
bool config_sensor_bme280_enabled;
uint8_t config_sensor_bme280_cs_pin;

void config_init(void);
void config_init_ephemeral(void);
void config_apply(void);
void config_save(void);

bool config_validate_eeprom(void);
bool config_validate_sd(void);

void config_wipe_eeprom(void);
void config_save_to_eeprom(void);
void config_save_to_sd(void);

void config_load_defaults(void);
void config_load_from_eeprom(void);
void config_load_from_sd(void);

void config_crypto_lock_enable(void);
void config_crypto_lock_disable(void);

void config_set_serial_baudrate(uint8_t baudrate);
void config_set_output_gain(uint8_t gain);
void config_set_input_gain(uint8_t gain);
void config_set_passall(uint8_t passall);
void config_set_log_packets(uint8_t log_packets);
void config_set_nmea_output(uint8_t nmea_output);
void config_set_gps_mode(uint8_t mode);
void config_set_bt_mode(uint8_t mode);
void config_set_invert_sddetect(uint8_t inv_sddetect);

void config_enable_diagnostics(void);
void config_disable_diagnostics(void);

void config_soft_reboot(void);
void config_print(void);

void EEPROM_updatebyte(uint16_t addr, uint8_t data);
uint8_t EEPROM_readbyte(uint16_t addr);
void EEPROM_writebyte(uint16_t addr, uint8_t data);

#endif

/*
	CSMA P
	CSMA Slot Time
	Preamble
	Tail

	LED intensity
	Output gain
	Input gain

	Pass-all

	Log packets
*/