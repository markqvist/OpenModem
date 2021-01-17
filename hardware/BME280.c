#include "BME280.h"
#include "hardware/LED.h"
#include "util/time.h"
#include "util/Config.h"

void bme280_cs(bool val) {
	if (!val) {
		// Pull CS line low to enable bus
		USR_IO_PORT &= ~(_BV(bme280_cs_usrio_pin));
	} else {
		// Set CS line to high to disable bus
		USR_IO_PORT |= _BV(bme280_cs_usrio_pin);
	}
}

uint8_t bme280_exchange(uint8_t tx) {
	SPDR = tx;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void bme280_deselect(void) {
	bme280_cs(true);
}

void bme280_select(void) {
	bme280_cs(false);
}

void bme280_exchange_multi(const uint8_t *buf, size_t len) {
	do {
		SPDR = *buf++; while(!(SPSR & (1<<SPIF)));
		SPDR = *buf++; while(!(SPSR & (1<<SPIF)));
	} while (len -= 2);
}

void bme280_receive_multi(uint8_t *buf, size_t len) {
	do {
		SPDR = 0xFF; loop_until_bit_is_set(SPSR,SPIF); *buf++ = SPDR;
		SPDR = 0xFF; loop_until_bit_is_set(SPSR,SPIF); *buf++ = SPDR;
	} while (len -= 2);
}

void bme280_write8(uint8_t reg_addr, uint8_t value) {
	bme280_select();
	bme280_exchange(reg_addr & ~0x80); // Write mode
	bme280_exchange(value);
	bme280_deselect();
}

uint8_t bme280_read8(uint8_t reg_addr) {
	bme280_select();
	bme280_exchange(reg_addr | 0x80); // Read mode
	uint8_t value = bme280_exchange(0x00);
	bme280_deselect();
	return value;
}

uint16_t bme280_read16(uint8_t reg_addr) {
	bme280_select();
	bme280_exchange(reg_addr | 0x80);
	uint16_t value = (bme280_exchange(0x00) << 8) | bme280_exchange(0x00);
	bme280_deselect();
	return value;
}

uint16_t bme280_read16_le(uint8_t reg_addr) {
	uint16_t value = bme280_read16(reg_addr);
	return (value >> 8) | (value << 8);
}

int16_t bme280_read16_s(uint8_t reg_addr) {
	return (int16_t)bme280_read16(reg_addr);
}

int16_t bme280_read16_s_le(uint8_t reg_addr) {
	return (int16_t)bme280_read16_le(reg_addr);
}

uint32_t bme280_read24(uint8_t reg_addr) {
	uint32_t value;

	bme280_select();
	bme280_exchange(reg_addr | 0x80); // Bit 7 high for read mode

	value = bme280_exchange(0x00);
	value = value << 8;
	value |= bme280_exchange(0x00);
	value = value << 8;
	value |= bme280_exchange(0x00);

	bme280_deselect();
	return value;
}

bool bme280_reading_calibration(void) {
	uint8_t const status = bme280_read8(BME280_REGISTER_STATUS);
	return (status & (1 << 0)) != 0;
}

void bme280_read_coefficients(void) {
	bme280_calibration.dig_T1 = bme280_read16_le(BME280_REGISTER_DIG_T1);
	bme280_calibration.dig_T2 = bme280_read16_s_le(BME280_REGISTER_DIG_T2);
	bme280_calibration.dig_T3 = bme280_read16_s_le(BME280_REGISTER_DIG_T3);

	bme280_calibration.dig_P1 = bme280_read16_le(BME280_REGISTER_DIG_P1);
	bme280_calibration.dig_P2 = bme280_read16_s_le(BME280_REGISTER_DIG_P2);
	bme280_calibration.dig_P3 = bme280_read16_s_le(BME280_REGISTER_DIG_P3);
	bme280_calibration.dig_P4 = bme280_read16_s_le(BME280_REGISTER_DIG_P4);
	bme280_calibration.dig_P5 = bme280_read16_s_le(BME280_REGISTER_DIG_P5);
	bme280_calibration.dig_P6 = bme280_read16_s_le(BME280_REGISTER_DIG_P6);
	bme280_calibration.dig_P7 = bme280_read16_s_le(BME280_REGISTER_DIG_P7);
	bme280_calibration.dig_P8 = bme280_read16_s_le(BME280_REGISTER_DIG_P8);
	bme280_calibration.dig_P9 = bme280_read16_s_le(BME280_REGISTER_DIG_P9);

	bme280_calibration.dig_H1 = bme280_read8(BME280_REGISTER_DIG_H1);
	bme280_calibration.dig_H2 = bme280_read16_s_le(BME280_REGISTER_DIG_H2);
	bme280_calibration.dig_H3 = bme280_read8(BME280_REGISTER_DIG_H3);

	bme280_calibration.dig_H4 = ((int8_t)bme280_read8(BME280_REGISTER_DIG_H4) << 4) |
		                        (bme280_read8(BME280_REGISTER_DIG_H4 + 1) & 0xF);

	bme280_calibration.dig_H5 = ((int8_t)bme280_read8(BME280_REGISTER_DIG_H5 + 1) << 4) |
                                (bme280_read8(BME280_REGISTER_DIG_H5) >> 4);

    bme280_calibration.dig_H6 = bme280_read8(BME280_REGISTER_DIG_H6);
}


void bme280_set_sampling(void) {
	uint8_t humidity_control_reg_values = BME280_SAMPLING_X16;
	uint8_t config_reg_values = BME280_STANDBY_MS_0_5 << 5 | BME280_FILTER_OFF << 2;
	uint8_t control_reg_values = BME280_SAMPLING_X16 << 5 | BME280_SAMPLING_X16 << 2 | BME280_MODE_NORMAL;

	bme280_write8(BME280_REGISTER_CONTROLHUMID, humidity_control_reg_values);
	bme280_write8(BME280_REGISTER_CONFIG, config_reg_values);
	bme280_write8(BME280_REGISTER_CONTROL, control_reg_values);

}

bool bme280_read_temperature(void) {
	int32_t var1, var2;
	int32_t adc_temp = bme280_read24(BME280_REGISTER_TEMPDATA);
	if (adc_temp == 0x800000) {
		bme280_temperature = BME280_TEMP_UNDEFINED;
		return false;
	} else {
		adc_temp >>= 4;

		var1 = ((((adc_temp >> 3) - ((int32_t)bme280_calibration.dig_T1 << 1))) *
          ((int32_t)bme280_calibration.dig_T2)) >>
         11;

        var2 = (((((adc_temp >> 4) - ((int32_t)bme280_calibration.dig_T1)) *
            ((adc_temp >> 4) - ((int32_t)bme280_calibration.dig_T1))) >>
           12) *
          ((int32_t)bme280_calibration.dig_T3)) >>
         14;

        bme280_temperature_fine = var1 + var2 + bme280_temperature_fine_adjust;

        float temperature = (bme280_temperature_fine * 5 + 128) >> 8;
        bme280_temperature = temperature / 100;

        return true;
	}

}

bool bme280_read_pressure(void) {
	int64_t var1, var2, p;

	bme280_read_temperature();
	int32_t adc_P = bme280_read24(BME280_REGISTER_PRESSUREDATA);

	if (adc_P == 0x800000) {
		bme280_pressure = BME280_PRESSURE_UNDEFINED;
		return false;
	}

	adc_P >>= 4;

	var1 = ((int64_t)bme280_temperature_fine) - 128000;
	var2 = var1 * var1 * (int64_t)bme280_calibration.dig_P6;
	var2 = var2 + ((var1 * (int64_t)bme280_calibration.dig_P5) << 17);
	var2 = var2 + (((int64_t)bme280_calibration.dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)bme280_calibration.dig_P3) >> 8) +
	       ((var1 * (int64_t)bme280_calibration.dig_P2) << 12);
	var1 =
	    (((((int64_t)1) << 47) + var1)) * ((int64_t)bme280_calibration.dig_P1) >> 33;

	if (var1 == 0) {
	  return false;
	}

	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)bme280_calibration.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)bme280_calibration.dig_P8) * p) >> 19;

	p = ((p + var1 + var2) >> 8) + (((int64_t)bme280_calibration.dig_P7) << 4);

	bme280_pressure = (float)p / 25600;
	return true;
}

bool bme280_read_humidity(void) {
	int32_t v_x1_u32r;

	bme280_read_temperature();

	int32_t adc_H = bme280_read16(BME280_REGISTER_HUMIDDATA);

	if (adc_H == 0x800000) {
		bme280_humidity = BME280_PRESSURE_UNDEFINED;
		return false;
	}

	v_x1_u32r = (bme280_temperature_fine - ((int32_t)76800));

	v_x1_u32r = (((((adc_H << 14) - (((int32_t)bme280_calibration.dig_H4) << 20) -
	                (((int32_t)bme280_calibration.dig_H5) * v_x1_u32r)) +
	               ((int32_t)16384)) >>
	              15) *
	             (((((((v_x1_u32r * ((int32_t)bme280_calibration.dig_H6)) >> 10) *
	                  (((v_x1_u32r * ((int32_t)bme280_calibration.dig_H3)) >> 11) +
	                   ((int32_t)32768))) >>
	                 10) +
	                ((int32_t)2097152)) *
	                   ((int32_t)bme280_calibration.dig_H2) +
	               8192) >>
	              14));

	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
	                           ((int32_t)bme280_calibration.dig_H1)) >>
	                          4));

	v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
	v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
	float h = (v_x1_u32r >> 12);
	bme280_humidity = h / 1024.0;
	
	return true;
}

bool bme280_init(uint8_t cs_usrio_pin) {
	bme280_installed = false;
	bme280_ready = false;
	bme280_cs_usrio_pin = cs_usrio_pin;

	bme280_temperature_fine_adjust = BME280_TEMPERATURE_ADJUSTMENT;

	USR_IO_DDR |= _BV(bme280_cs_usrio_pin);

	SPI_DDR |= _BV(SPI_MOSI) | _BV(SPI_CLK);
    SPI_DDR &= ~(_BV(SPI_MISO));
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);

    bme280_deselect();
    uint8_t sensor_id = bme280_read8(BME280_REGISTER_CHIPID);
    if (sensor_id == 0x60) {
    	bme280_installed = true;

    	// Soft-reset chip to clear settings
    	bme280_write8(BME280_REGISTER_SOFTRESET, 0xB6);
    	delay_ms(10);

    	// Wait for chip to read calibration data
    	while (bme280_reading_calibration()) {
    		delay_ms(10);
    	}

    	// Load calibration coefficients
    	bme280_read_coefficients();

    	// Set sampling configuration
    	bme280_set_sampling();
    	delay_ms(100);

    	bme280_ready = true;
    	return true;
    } else {
    	bme280_installed = false;
    	bme280_ready = false;
    	return false;
    }
}

bool bme280_poll(void) {
	if (bme280_read_temperature() && bme280_read_pressure() && bme280_read_humidity()) {
		return true;
	} else {
		return false;
	}
}