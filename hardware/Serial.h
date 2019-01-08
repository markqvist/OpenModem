#ifndef SERIAL_H
#define SERIAL_H

#include "device.h"

#include <stdio.h>
#include <stdbool.h>
#include <avr/io.h>
#include "util/FIFO.h"

typedef struct Serial {
    FILE uart0;
} Serial;

FIFOBuffer serialFIFO;
uint8_t serialBuf[CONFIG_SERIAL_BUFFER_SIZE];

void serial_init(Serial *serial);
bool serial_available(uint8_t index);
int uart0_putchar(char c, FILE *stream);
int uart0_getchar(FILE *stream);
char uart0_getchar_nowait(void);

#endif