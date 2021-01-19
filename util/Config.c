#include <util/atomic.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include "Config.h"
#include "device.h"
#include "hardware/crypto/MD5.h"
#include "hardware/AFSK.h"
#include "hardware/Serial.h"
#include "hardware/VREF.h"
#include "protocol/KISS.h"


void config_init(void) {
	config_source = CONFIG_SOURCE_NONE;
	config_output_diagnostics = false;

	bool has_valid_eeprom_config = config_validate_eeprom();

	if (has_valid_eeprom_config) {
		config_load_from_eeprom();
	} else {
		config_load_defaults();
		config_save_to_eeprom();
	}

	config_init_ephemeral();
	config_apply();
}

void config_init_ephemeral(void) {
	config_user_jobs_enabled = CONFIG_USER_JOBS_ENABLED;
	config_gps_nmea_output = CONFIG_GPS_NMEA_NONE;
	config_sensors_enabled = CONFIG_SENSORS_ENABLED;
	config_sensor_interval_ms = CONFIG_SENSORS_INTERVAL_MS;
	config_sensor_bme280_enabled = CONFIG_BME280_ENABLED;
	config_sensor_bme280_cs_pin = USR_IO_1;
}

void config_apply(void) {
	if (config_serial_baudrate == CONFIG_BAUDRATE_1200) serial_setbaudrate_1200(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_2400) serial_setbaudrate_2400(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_4800) serial_setbaudrate_4800(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_9600) serial_setbaudrate_9600(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_14400) serial_setbaudrate_14400(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_19200) serial_setbaudrate_19200(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_28800) serial_setbaudrate_28800(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_38400) serial_setbaudrate_38400(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_57600) serial_setbaudrate_57600(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_76800) serial_setbaudrate_76800(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_115200) serial_setbaudrate_115200(0);
	if (config_serial_baudrate == CONFIG_BAUDRATE_230400) serial_setbaudrate_230400(0);
}

void config_save(void) {
	config_save_to_eeprom();
	config_save_to_sd();
}

void config_wipe_eeprom(void) {
	for (uint16_t i = 0; i < ADDR_E_END; i++) {
		EEPROM_updatebyte(i, 0x00);
	}
}

bool config_validate_eeprom(void) {
	uint8_t config_size = ADDR_E_END;
	uint8_t config_data_size = ADDR_E_END - CONF_CHECKSUM_SIZE;
	uint8_t config_data[config_data_size];
	uint8_t config_checksum[CONF_CHECKSUM_SIZE];

	for (uint16_t addr = 0; addr < config_data_size; addr++) {
		config_data[addr] = EEPROM_readbyte(addr);
	}

	for (uint16_t addr = config_data_size; addr < config_size; addr++) {
		config_checksum[addr-config_data_size] = EEPROM_readbyte(addr);
	}

	md5_hash_t calculated_checksum;
	md5(&calculated_checksum, &config_data, config_data_size*8);

	bool checksums_match = true;
	for (uint8_t i = 0; i < CONF_CHECKSUM_SIZE; i++) {
		if (calculated_checksum[i] != config_checksum[i]) {
			checksums_match = false;
			break;
		}
	}

	if (checksums_match) {
		return true;
	} else {
		return false;
	}
}

void config_save_to_eeprom(void) {
	uint8_t i = 0;
	uint8_t config_size = ADDR_E_END;
	uint8_t config_data_size = ADDR_E_END - CONF_CHECKSUM_SIZE;
	uint8_t config_data[config_size];
	memset(config_data, 0x00, ADDR_E_END);

	config_data[i++] = MAJ_VERSION;
	config_data[i++] = MIN_VERSION;
	config_data[i++] = CONF_VERSION;
	config_data[i++] = config_p;
	config_data[i++] = config_slottime/10;
	config_data[i++] = config_preamble/10;
	config_data[i++] = config_tail/10;
	config_data[i++] = config_led_intensity;
	config_data[i++] = config_output_gain;
	config_data[i++] = config_input_gain;
	config_data[i++] = config_passall;
	config_data[i++] = config_log_packets;
	config_data[i++] = config_crypto_lock;
	config_data[i++] = config_gps_mode;
	config_data[i++] = config_bluetooth_mode;
	config_data[i++] = config_serial_baudrate;
	config_data[i++] = config_invert_sddetect;

	md5_hash_t checksum;
	md5(&checksum, &config_data, config_data_size*8);
	for (uint8_t j = 0; j < CONF_CHECKSUM_SIZE; j++) {
		config_data[i++] = checksum[j];
	}

	for (uint16_t addr = 0; addr < config_size; addr++) {
		EEPROM_updatebyte(addr, config_data[addr]);
	}
}

void config_load_defaults(void) {
	config_p = CONFIG_CSMA_P_DEFAULT;
	config_slottime = CONFIG_CSMA_SLOTTIME_DEFAULT;
	config_preamble = CONFIG_AFSK_PREAMBLE_LEN;
	config_tail = CONFIG_AFSK_TRAILER_LEN;
	config_led_intensity = CONFIG_LED_INTENSITY;
	config_output_gain = CONFIG_DAC_REF;
	config_input_gain = CONFIG_ADC_REF;
	config_passall = false;
	config_log_packets = false;
	config_crypto_lock = false;
	config_gps_mode = CONFIG_GPS_AUTODETECT;
	config_bluetooth_mode = CONFIG_BLUETOOTH_AUTODETECT;
	config_serial_baudrate = CONFIG_BAUDRATE_115200;
	config_source = CONFIG_SOURCE_DEFAULT;
	config_invert_sddetect = false;
}

void config_load_from_eeprom(void) {
	uint8_t config_data_size = ADDR_E_END - CONF_CHECKSUM_SIZE;
	uint8_t config_data[config_data_size];

	for (uint16_t addr = 0; addr < config_data_size; addr++) {
		config_data[addr] = EEPROM_readbyte(addr);
	}

	config_p = config_data[ADDR_E_P];
	config_slottime = config_data[ADDR_E_SLOTTIME]*10UL;
	config_preamble = config_data[ADDR_E_PREAMBLE]*10UL;
	config_tail = config_data[ADDR_E_TAIL]*10UL;
	config_led_intensity = config_data[ADDR_E_LED_INTENSITY];
	config_output_gain = config_data[ADDR_E_OUTPUT_GAIN];
	config_input_gain = config_data[ADDR_E_INPUT_GAIN];
	config_passall = config_data[ADDR_E_PASSALL];
	config_log_packets = config_data[ADDR_E_LOG_PACKETS];
	config_crypto_lock = config_data[ADDR_E_CRYPTO_LOCK];
	config_gps_mode = config_data[ADDR_E_GPS_MODE];
	config_bluetooth_mode = config_data[ADDR_E_BLUETOOTH_MODE];
	config_serial_baudrate = config_data[ADDR_E_SERIAL_BAUDRATE];
	config_invert_sddetect = config_data[ADDR_E_INVERT_SDDETECT];
}

bool config_validate_sd(void) {
	// TODO: Implement
	return false;
}

void config_save_to_sd(void) {
	// TODO: Implement
}

void config_load_from_sd(void) {
	// TODO: Implement
	return;
}

void config_crypto_lock_enable(void) {
	config_crypto_lock = true;
	config_save_to_eeprom();
}

void config_crypto_lock_disable(void) {
	config_crypto_lock = false;
	config_soft_reboot();
}

void config_set_serial_baudrate(uint8_t baudrate) {
	if (baudrate >= CONFIG_BAUDRATE_1200 && baudrate <= CONFIG_BAUDRATE_230400) {
		config_serial_baudrate = baudrate;
	}
}

void config_set_output_gain(uint8_t gain) {
	vref_setDAC(gain);
}

void config_set_input_gain(uint8_t gain) {
	vref_setADC(gain);
}

void config_set_passall(uint8_t passall) {
	if (passall == 0x00) {
		config_passall = false;
	} else {
		config_passall = true;
	}
}

void config_set_log_packets(uint8_t log_packets) {
	if (log_packets == 0x00) {
		config_log_packets = false;
	} else {
		config_log_packets = true;
	}
}

void config_set_gps_mode(uint8_t mode) {
	if (mode == CONFIG_GPS_OFF) config_gps_mode = CONFIG_GPS_OFF;
	if (mode == CONFIG_GPS_AUTODETECT) config_gps_mode = CONFIG_GPS_AUTODETECT;
	if (mode == CONFIG_GPS_REQUIRED) config_gps_mode = CONFIG_GPS_REQUIRED;
}

void config_set_nmea_output(uint8_t nmea_output) {
	if (nmea_output == CONFIG_GPS_NMEA_NONE) config_gps_nmea_output = nmea_output;
	if (nmea_output == CONFIG_GPS_NMEA_RAW) config_gps_nmea_output = nmea_output;
	if (nmea_output == CONFIG_GPS_NMEA_ENCAP) config_gps_nmea_output = nmea_output;
}

void config_set_bt_mode(uint8_t mode) {
	if (mode == CONFIG_BLUETOOTH_OFF) config_bluetooth_mode = CONFIG_BLUETOOTH_OFF;
	if (mode == CONFIG_BLUETOOTH_AUTODETECT) config_bluetooth_mode = CONFIG_BLUETOOTH_AUTODETECT;
	if (mode == CONFIG_BLUETOOTH_REQUIRED) config_bluetooth_mode = CONFIG_BLUETOOTH_REQUIRED;
}

void config_set_invert_sddetect(uint8_t invert_sddetect) {
	if (invert_sddetect == 0x00) {
		config_invert_sddetect = false;
	} else {
		config_invert_sddetect = true;
	}
}

void EEPROM_writebyte(uint16_t addr, uint8_t data) {
	// Disable interrupts
    cli();

    // Wait for EEPROM ready
    while(EECR & (1<<EEPE));

    // Set up address and data registers
    EEAR = addr;
    EEDR = data;

    // Set EEPROM write enable bit
    EECR |= (1<<EEMPE);

    // Start EEPROM write, interrupts
    // are enabled after write
    sei();
    EECR |= (1<<EEPE);
}

uint8_t EEPROM_readbyte(uint16_t addr) {
	// Disable interrupts
    cli();

    // Wait for EEPROM ready
    while(EECR & (1<<EEPE));

    // Set up address and data registers
    EEAR = addr;
    EECR |= (1<<EERE);

    // Enable interrupts and fetch result
    sei();
    uint8_t byte = EEDR;

    return byte;
}

void EEPROM_updatebyte(uint16_t addr, uint8_t data) {
	uint8_t byte = EEPROM_readbyte(addr);
	if (byte != data) {
		EEPROM_writebyte(addr, data);
	}
}

void config_soft_reboot(void) {
	config_save_to_eeprom();
	wdt_enable(WDTO_15MS);
	while(true) { }
}

void config_enable_diagnostics(void) {
	config_output_diagnostics = true;
}

void config_disable_diagnostics(void) {
	config_output_diagnostics = false;
}

// TODO: remove this
void config_print(void) {
	uint8_t config_size = ADDR_E_END;
	uint8_t config_data[config_size];

	for (uint16_t addr = 0; addr < config_size; addr++) {
		config_data[addr] = EEPROM_readbyte(addr);
	}

	kiss_output_config(config_data, config_size);

	// printf(PSTR("Running configuration:\r\n"));
	// printf("\tP\t\t%02X\r\n", config_p);
	// printf("\tSlottime\t%lu\r\n", config_slottime);
	// printf("\tPreamble\t%lu\r\n", config_preamble);
	// printf("\tTail\t\t%lu\r\n", config_tail);
	// printf("\tLEDs\t\t%02X\r\n", config_led_intensity);
	// printf("\tOut gain\t%02X\r\n", config_output_gain);
	// printf("\tIn gain\t\t%02X\r\n", 0xFF - config_input_gain);
	// printf("\tPassall\t\t%02X\r\n", config_passall);
	// printf("\tLog pkts\t%02X\r\n", config_log_packets);
	// printf("\tCrypto lock\t%02X\r\n", config_crypto_lock);
	// printf("\tGPS mode\t%02X\r\n", config_gps_mode);
	// printf("\tBT Mode\t\t%02X\r\n", config_bluetooth_mode);
	// printf("\tBaudrate\t%02X\r\n", config_serial_baudrate);
}