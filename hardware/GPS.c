#include "GPS.h"
#include "util/Config.h"
#include "protocol/KISS.h"

#include <time.h>

Serial *serial;

bool gps_installed = false;
bool gps_power = true;

uint8_t nmea_read_length = 0;
char nmea_input_buf[NMEA_MAX_LENGTH];
char nmea_parse_buf[NMEA_MAX_LENGTH];

bool gps_detect(void) {
	GPS_DDR &= ~_BV(GPS_EN_PIN);
	return (GPS_INPUT & _BV(GPS_EN_PIN));
}

void gps_powerup(void) {
	GPS_PORT |= _BV(GPS_EN_PIN);
}

void gps_powerdown(void) {
	GPS_PORT &= ~_BV(GPS_EN_PIN);
}

bool gps_enabled(void) {
	return gps_power;
}

void gps_init(Serial *ser) {
	serial = ser;
	memset(nmea_input_buf, 0, sizeof(nmea_input_buf));
	memset(nmea_parse_buf, 0, sizeof(nmea_parse_buf));

	gps_speed_knots = 0;
	gps_speed_kmh = 0;
	gps_bearing = 0;

	gps_fix = false;
	gps_time_set = false;

	if (gps_detect()) {
		gps_installed = true;
		GPS_DDR |= _BV(GPS_EN_PIN);

		if (config_gps_mode == CONFIG_GPS_AUTODETECT || config_gps_mode == CONFIG_GPS_REQUIRED) {
			gps_powerup();

			serial_setbaudrate_9600(1);
			delay_ms(100);

			gps_send_command(PMTK_SET_BAUD_57600);
			delay_ms(100);
			
			serial_setbaudrate_57600(1);
			delay_ms(100);
			
			gps_send_command(PMTK_API_SET_FIX_CTL_1HZ);
			gps_send_command(PMTK_SET_NMEA_OUTPUT_RMCGGA);

			gps_power = true;
		} else {
			gps_power = false;
			gps_powerdown();
		}

	} else {
		gps_installed = false;
		gps_power = false;
	}
}


void gps_update_rtc(void) {
	struct tm now;

	now.tm_year = (gps_t_year+2000) - 1900;
	now.tm_mon = gps_t_month-1;
	now.tm_mday = gps_t_day;
	now.tm_hour = gps_t_hour;
	now.tm_min = gps_t_minute;
	now.tm_sec = gps_t_second;
	now.tm_isdst = -1;

	time_t timestamp = mktime(&now);
	rtc_set_seconds(timestamp);
	gps_time_set = true;
}

void gps_jobs(void) {
	if (gps_enabled()) { gps_powerup(); } else { gps_powerdown(); }
}

void gps_send_command(const char *cmd) {
	fprintf_P(&serial->uart1, cmd);
}

void gps_nmea_parse(uint8_t sentence_length) {
	if (config_gps_nmea_output == CONFIG_GPS_NMEA_RAW) {
		printf("%s\n\r", nmea_parse_buf);
	}

	if (config_gps_nmea_output == CONFIG_GPS_NMEA_ENCAP) {
		kiss_output_nmea(nmea_parse_buf, sentence_length);	
	}

	if (sentence_length > 4) {
		if (nmea_parse_buf[sentence_length-4] == '*') {
			uint16_t checksum = gps_nmea_parse_hex(nmea_parse_buf[sentence_length-3]);
			checksum *= 16;
			checksum += gps_nmea_parse_hex(nmea_parse_buf[sentence_length-2]);

			for (uint8_t i=1; i < sentence_length-4; i++) {
				checksum ^= nmea_parse_buf[i];
			}

			if (checksum != 0) {
				return;
			} else {
				// Parse GGA sentence
				if (strstr(nmea_parse_buf, "$GPGGA")) {
					char *pointer = nmea_parse_buf;
					
					// Ignore UTC time
					pointer = strchr(pointer, ',')+1;

					// Parse latitude
					pointer = strchr(pointer, ',')+1;
					char nmea_lat_deg_str[3];
					nmea_lat_deg_str[0] = pointer[0];
					nmea_lat_deg_str[1] = pointer[1];
					nmea_lat_deg_str[2] = '\0';
					
					char nmea_lat_min_str[3];
					nmea_lat_min_str[0] = pointer[2];
					nmea_lat_min_str[1] = pointer[3];
					nmea_lat_min_str[2] = '\0';

					char nmea_lat_dec_str[5];
					nmea_lat_dec_str[0] = pointer[5];
					nmea_lat_dec_str[1] = pointer[6];
					nmea_lat_dec_str[2] = pointer[7];
					nmea_lat_dec_str[3] = pointer[8];
					nmea_lat_dec_str[4] = '\0';

					// Parse latitude sign
					pointer = strchr(pointer, ',')+1;
					char nmea_lat_sign = pointer[0];

					// Parse longtitude
					pointer = strchr(pointer, ',')+1;
					char nmea_lon_deg_str[4];
					nmea_lon_deg_str[0] = pointer[0];
					nmea_lon_deg_str[1] = pointer[1];
					nmea_lon_deg_str[2] = pointer[2];
					nmea_lon_deg_str[3] = '\0';

					char nmea_lon_min_str[3];
					nmea_lon_min_str[0] = pointer[3];
					nmea_lon_min_str[1] = pointer[4];
					nmea_lon_min_str[2] = '\0';

					char nmea_lon_dec_str[5];
					nmea_lon_dec_str[0] = pointer[6];
					nmea_lon_dec_str[1] = pointer[7];
					nmea_lon_dec_str[2] = pointer[8];
					nmea_lon_dec_str[3] = pointer[9];
					nmea_lon_dec_str[4] = '\0';

					// Parse longtitude sign
					pointer = strchr(pointer, ',')+1;
					char nmea_lon_sign = pointer[0];

					// Get fix quality
					pointer = strchr(pointer, ',')+1;
					uint8_t nmea_fix = atoi(pointer);
					if (nmea_fix > 0 && nmea_fix < 7) {
						gps_fix = true;
					} else {
						gps_fix = false;
					}

					if (gps_fix) {
						// Set latitude and longtitude
						gps_lat_degrees = atoi(nmea_lat_deg_str);
						gps_lat_minutes = atoi(nmea_lat_min_str);
						gps_lat_seconds = (atoi(nmea_lat_dec_str)/10000.0)*60.0;
						gps_lon_degrees = atoi(nmea_lon_deg_str);
						gps_lon_minutes = atoi(nmea_lon_min_str);
						gps_lon_seconds = (atoi(nmea_lon_dec_str)/10000.0)*60.0;
						gps_lat = gps_lat_degrees + (gps_lat_minutes/60.0) + (gps_lat_seconds/3600.0);
						gps_lon = gps_lon_degrees + (gps_lon_minutes/60.0) + (gps_lon_seconds/3600.0);
						
						// Get satellites
						pointer = strchr(pointer, ',')+1;
						gps_sats = atoi(pointer);

						// Get horizontal dilution of position
						pointer = strchr(pointer, ',')+1;
						gps_hdop = atof(pointer);

						// Get altitude
						pointer = strchr(pointer, ',')+1;
						gps_altitude = atof(pointer);

						// Ignore altitude unit reference
						pointer = strchr(pointer, ',')+1;

						// Get geoid height
						pointer = strchr(pointer, ',')+1;
						gps_geoid_height = atof(pointer);
						// TODO: Check this calculation
						gps_height_above_msl = gps_geoid_height + gps_altitude;

						// Ignore geoid height unit reference
						pointer = strchr(pointer, ',')+1;
						
						gps_lat_sign = nmea_lat_sign;
						gps_lon_sign = nmea_lon_sign;
						gps_lat *= (gps_lat_sign == 'N' ? 1 : -1);
						gps_lon *= (gps_lon_sign == 'E' ? 1 : -1);						
					}
				}

				// Parse RMC sentence
				if (strstr(nmea_parse_buf, "$GPRMC")) {
					if (gps_fix) {
						char *pointer = nmea_parse_buf;
						uint32_t nmea_date = 0;
						uint32_t nmea_time = 0;

						// Get UTC time
						pointer = strchr(pointer, ',')+1;
						if (!gps_time_set) nmea_time = (float)atof(pointer);
						
						// Ignore navigation receiver warning
						pointer = strchr(pointer, ',')+1;

						// Ignore latitude
						pointer = strchr(pointer, ',')+1;

						// Ignore latitude sign
						pointer = strchr(pointer, ',')+1;

						// Ignore longtitude
						pointer = strchr(pointer, ',')+1;

						// Ignore longtitude sign
						pointer = strchr(pointer, ',')+1;

						// Get ground speed
						pointer = strchr(pointer, ',')+1;
						gps_speed_knots = atof(pointer);
						gps_speed_kmh = gps_speed_knots * 1.852;

						// Get bearing
						pointer = strchr(pointer, ',')+1;
						gps_bearing = atof(pointer);

						// Get date
						pointer = strchr(pointer, ',')+1;
						if (!gps_time_set) nmea_date = (float)atof(pointer);

						// Set times
						if (!gps_time_set) {
							gps_t_hour    = nmea_time / 10000;
							gps_t_minute  = (nmea_time % 10000) / 100;
							gps_t_second  = (nmea_time % 100);
							gps_t_day     = nmea_date / 10000;
							gps_t_month   = (nmea_date % 10000) / 100;
							gps_t_year    = (nmea_date % 100);
							gps_update_rtc();
						}
						
					} else {
						gps_speed_knots = 0;
						gps_speed_kmh = 0;
					}
				}
			}
		}
	}
}

void gps_serial_callback(char byte) {
	if (byte == '\n') {
		for (uint8_t i = 0; i < NMEA_MAX_LENGTH; i++)
			nmea_parse_buf[i] = nmea_input_buf[i];
		
		memset(nmea_input_buf, 0, sizeof(nmea_input_buf));

		gps_nmea_parse(nmea_read_length);
		memset(nmea_parse_buf, 0, sizeof(nmea_parse_buf));

		nmea_read_length = 0;
	} else {
		if (nmea_read_length < NMEA_MAX_LENGTH) {
			nmea_input_buf[nmea_read_length++] = byte;
		}
	}
}

void gps_poll(void) {
    while (!fifo_isempty_locked(&uart1FIFO)) {
        char sbyte = fifo_pop_locked(&uart1FIFO);
        gps_serial_callback(sbyte);
    }
}

uint8_t gps_nmea_parse_hex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;

    return 0;
}
