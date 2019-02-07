#ifndef SD_H
#define SD_H

#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include "device.h"
#include "util/time.h"
#include "hardware/UserIO.h"
#include "hardware/sdcard/ff.h"

#define SD_STATUS_READY			  0x00
#define SD_STATUS_NOINIT		  0x01
#define SD_STATUS_NODISK		  0x02
#define SD_STATUS_WRITEPROTECTED  0x04
#define SD_STATUS_UNKNOWN		  0xFF

void sd_init(void);
void sd_test(void);
void sd_scheduler(void);
void sd_jobs(void);
void sd_automount(void);
void sd_autounmount(void);
void sd_statuschange_indication(uint8_t pattern);

bool sd_mounted(void);

#endif