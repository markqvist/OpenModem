#ifndef bme280_H
#define bme280_H

#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "device.h"
#include "util/time.h"

#define BME280_TEMPERATURE_ADJUSTMENT 0

#define BME280_TEMP_UNDEFINED -274
#define BME280_PRESSURE_UNDEFINED -1
#define BME280_HUMIDITY_UNDEFINED -1

#define BME280_REGISTER_DIG_T1 0x88
#define BME280_REGISTER_DIG_T2 0x8A
#define BME280_REGISTER_DIG_T3 0x8C
#define BME280_REGISTER_DIG_P1 0x8E
#define BME280_REGISTER_DIG_P2 0x90
#define BME280_REGISTER_DIG_P3 0x92
#define BME280_REGISTER_DIG_P4 0x94
#define BME280_REGISTER_DIG_P5 0x96
#define BME280_REGISTER_DIG_P6 0x98
#define BME280_REGISTER_DIG_P7 0x9A
#define BME280_REGISTER_DIG_P8 0x9C
#define BME280_REGISTER_DIG_P9 0x9E
#define BME280_REGISTER_DIG_H1 0xA1
#define BME280_REGISTER_DIG_H2 0xE1
#define BME280_REGISTER_DIG_H3 0xE3
#define BME280_REGISTER_DIG_H4 0xE4
#define BME280_REGISTER_DIG_H5 0xE5
#define BME280_REGISTER_DIG_H6 0xE7
#define BME280_REGISTER_CHIPID 0xD0
#define BME280_REGISTER_VERSION 0xD1
#define BME280_REGISTER_SOFTRESET 0xE0
#define BME280_REGISTER_CAL26 0xE1 // R calibration stored in 0xE1-0xF0
#define BME280_REGISTER_CONTROLHUMID 0xF2
#define BME280_REGISTER_STATUS 0XF3
#define BME280_REGISTER_CONTROL 0xF4
#define BME280_REGISTER_CONFIG 0xF5
#define BME280_REGISTER_PRESSUREDATA 0xF7
#define BME280_REGISTER_TEMPDATA 0xFA
#define BME280_REGISTER_HUMIDDATA 0xFD

#define BME280_SAMPLING_NONE 0b000
#define BME280_SAMPLING_X1 0b001
#define BME280_SAMPLING_X2 0b010
#define BME280_SAMPLING_X4 0b011
#define BME280_SAMPLING_X8 0b100
#define BME280_SAMPLING_X16 0b101

#define BME280_MODE_SLEEP 0b00
#define BME280_MODE_FORCED 0b01
#define BME280_MODE_NORMAL 0b11

#define BME280_FILTER_OFF 0b000
#define BME280_FILTER_X2 0b001
#define BME280_FILTER_X4 0b010
#define BME280_FILTER_X8 0b011
#define BME280_FILTER_X16 0b100

#define BME280_STANDBY_MS_0_5 0b000
#define BME280_STANDBY_MS_10 0b110
#define BME280_STANDBY_MS_20 0b111
#define BME280_STANDBY_MS_62_5 0b001
#define BME280_STANDBY_MS_125 0b010
#define BME280_STANDBY_MS_250 0b011
#define BME280_STANDBY_MS_500 0b100
#define BME280_STANDBY_MS_1000 0b101


typedef struct {
  uint16_t dig_T1; // temperature compensation value
  int16_t dig_T2;  // temperature compensation value
  int16_t dig_T3;  // temperature compensation value

  uint16_t dig_P1; // pressure compensation value
  int16_t dig_P2;  // pressure compensation value
  int16_t dig_P3;  // pressure compensation value
  int16_t dig_P4;  // pressure compensation value
  int16_t dig_P5;  // pressure compensation value
  int16_t dig_P6;  // pressure compensation value
  int16_t dig_P7;  // pressure compensation value
  int16_t dig_P8;  // pressure compensation value
  int16_t dig_P9;  // pressure compensation value

  uint8_t dig_H1;  // humidity compensation value
  int16_t dig_H2;  // humidity compensation value
  uint8_t dig_H3;  // humidity compensation value
  int16_t dig_H4;  // humidity compensation value
  int16_t dig_H5;  // humidity compensation value
  int8_t dig_H6;   // humidity compensation value
} bme280_calib_data;

bool bme280_init(uint8_t cs_usrio_pin);
bool bme280_poll(void);

bool bme280_installed;
bool bme280_ready;
uint8_t bme280_cs_usrio_pin;

int32_t bme280_temperature_fine_adjust;
int32_t bme280_temperature_fine;

float bme280_temperature;
float bme280_pressure;
float bme280_humidity;

bme280_calib_data bme280_calibration;

#endif