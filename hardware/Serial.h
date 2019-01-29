#ifndef SERIAL_H
#define SERIAL_H

#include "device.h"

#include <stdio.h>
#include <stdbool.h>
#include <avr/io.h>
#include "util/FIFO.h"
#include "hardware/LED.h"

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

void serial_setbaudrate_1200(void);
void serial_setbaudrate_2400(void);
void serial_setbaudrate_4800(void);
void serial_setbaudrate_9600(void);
void serial_setbaudrate_14400(void);
void serial_setbaudrate_19200(void);
void serial_setbaudrate_28800(void);
void serial_setbaudrate_38400(void);
void serial_setbaudrate_57600(void);
void serial_setbaudrate_76800(void);
void serial_setbaudrate_115200(void);
void serial_setbaudrate_230400(void);

#endif