#include "GPS.h"

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

	if (gps_detect()) {
		gps_installed = true;

		serial_setbaudrate_9600(1);
		gps_send_command(PMTK_SET_BAUD_57600);
		serial_setbaudrate_57600(1);

		gps_send_command(PMTK_API_SET_FIX_CTL_1HZ);
		gps_send_command(PMTK_SET_NMEA_OUTPUT_RMCGGA);

		GPS_DDR |= _BV(GPS_EN_PIN);
		if (gps_enabled()) { gps_powerup(); } else { gps_powerdown(); }
		
	} else {
		gps_installed = false;
		gps_power = false;
	}
}

void gps_update_rtc(void) {
	// TODO: implement this
}

void gps_jobs(void) {
	if (gps_enabled()) { gps_powerup(); } else { gps_powerdown(); }
}

void gps_send_command(const char *cmd) {
	fprintf_P(&serial->uart1, cmd);
}

void gps_nmea_parse(uint8_t sentence_length) {
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
					
					// Parse UTC time
					pointer = strchr(pointer, ',')+1;
					uint32_t nmea_time = (float)atof(pointer);

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
					if (nmea_fix == 1) {
						gps_fix = true;
					} else {
						gps_fix = false;
					}

					if (gps_fix) {
						// Set times
						gps_t_hour = nmea_time / 10000;
						gps_t_minute = (nmea_time % 10000) / 100;
						gps_t_second = (nmea_time % 100);
						gps_update_rtc();

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

						// TODO: Remove this
						// printf("GPS fix: %d\r\n", nmea_fix);
						// printf("GPS satellites: %d\r\n", gps_sats);
						// printf("GPS latitude: %d\" %d' %.2fs %c\r\n", gps_lat_degrees, gps_lat_minutes, gps_lat_seconds, gps_lat_sign);
						// printf("GPS longtitude: %d\" %d' %.2fs %c\r\n", gps_lon_degrees, gps_lon_minutes, gps_lon_seconds, gps_lon_sign);
						// printf("GPS coords: %.6f,%.6f\r\n", gps_lat, gps_lon);
						// printf("GPS speed %.2f Km/h\r\n", gps_speed_kmh);
						// printf("GPS speed %.2f knots\r\n", gps_speed_knots);
						// printf("GPS bearing %.2f\r\n", gps_bearing);
						// printf("GPS height above MSL: %.2f\r\n", gps_height_above_msl);
						// printf("GPS altitude: %.2f\r\n", gps_altitude);
						// printf("GPS geoid height: %.2f\r\n", gps_geoid_height);
						// printf("GPS HDOP: %.2f\r\n", gps_hdop);
						// printf("GPS time %d:%d:%d UTC\r\n", gps_t_hour, gps_t_minute, gps_t_second);
						
					}
				}

				// Parse RMC sentence
				if (strstr(nmea_parse_buf, "$GPRMC")) {
					if (gps_fix) {
						char *pointer = nmea_parse_buf;

						// Ignore UTC time
						pointer = strchr(pointer, ',')+1;

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
