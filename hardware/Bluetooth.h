#ifndef BT_H
#define BT_H

#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "device.h"
#include "util/time.h"
#include "hardware/serial.h"

void bluetooth_init(void);
bool bluetooth_enabled(void);


#endif

// CMD/DATA sequence: 0x2b2b2b0d
