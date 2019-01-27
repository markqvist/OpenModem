#ifndef SD_H
#define SD_H

#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include "device.h"
#include "util/time.h"
#include "hardware/sdcard/ff.h"

void sd_init(void);
void sd_test(void);

#endif