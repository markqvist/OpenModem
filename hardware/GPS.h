#ifndef GPS_H
#define GPS_H

#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "device.h"
#include "util/time.h"
#include "hardware/serial.h"

#define NMEA_MAX_LENGTH 128

#define PMTK_SET_BAUD_57600 PSTR("$PMTK251,57600*2C\r\n")
#define PMTK_SET_BAUD_9600 PSTR("$PMTK251,9600*17\r\n")
#define PMTK_SET_NMEA_OUTPUT_RMCONLY PSTR("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n")
#define PMTK_SET_NMEA_OUTPUT_RMCGGA PSTR("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n")
#define PMTK_SET_NMEA_OUTPUT_NONE PSTR("$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n")
#define PMTK_SET_NMEA_OUTPUT_ALL PSTR("$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n")
#define PMTK_API_SET_FIX_CTL_200_mHZ  PSTR("$PMTK300,5000,0,0,0,0*18\r\n")
#define PMTK_API_SET_FIX_CTL_1HZ  PSTR("$PMTK300,1000,0,0,0,0*1C\r\n")
#define PGCMD_ANTENNA PSTR("$PGCMD,33,1*6C\r\n")
#define PGCMD_NOANTENNA PSTR("$PGCMD,33,0*6D\r\n")

void gps_init(Serial *ser);
bool gps_enabled(void);
void gps_poll(void);
void gps_jobs(void);
void gps_send_command(const char *cmd);
uint8_t gps_parse_nmea(char *nmea);
uint8_t gps_nmea_parse_hex(char c);

uint8_t gps_t_year;
uint8_t gps_t_month;
uint8_t gps_t_day;
uint8_t gps_t_hour;
uint8_t gps_t_minute;
uint8_t gps_t_second;

int gps_lat_degrees;
int gps_lat_minutes;
float gps_lat_seconds;
float gps_lat;

int gps_lon_degrees;
int gps_lon_minutes;
float gps_lon_seconds;
float gps_lon;

float gps_geoid_height;
float gps_altitude;
float gps_height_above_msl;
float gps_speed_knots;
float gps_speed_kmh;
float gps_speed_mph;
float gps_bearing;
float gps_magvariation;
float gps_hdop;
char gps_lat_sign;
char gps_lon_sign;
char gps_mag_char;
bool gps_fix;
uint8_t gps_fix_quality;
uint8_t gps_sats;

#endif