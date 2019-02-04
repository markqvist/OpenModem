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
    FILE uart1;
} Serial;

FIFOBuffer uart0FIFO;
uint8_t uart0Buf[CONFIG_UART0_BUFFER_SIZE];

FIFOBuffer uart1FIFO;
uint8_t uart1Buf[CONFIG_UART1_BUFFER_SIZE];

void serial_init(Serial *serial);
bool serial_available(uint8_t index);

int uart0_putchar(char c, FILE *stream);
int uart0_getchar(FILE *stream);
char uart0_getchar_nowait(void);

int uart1_putchar(char c, FILE *stream);
int uart1_getchar(FILE *stream);
char uart1_getchar_nowait(void);

void serial_setbaudrate_1200(uint8_t port);
void serial_setbaudrate_2400(uint8_t port);
void serial_setbaudrate_4800(uint8_t port);
void serial_setbaudrate_9600(uint8_t port);
void serial_setbaudrate_14400(uint8_t port);
void serial_setbaudrate_19200(uint8_t port);
void serial_setbaudrate_28800(uint8_t port);
void serial_setbaudrate_38400(uint8_t port);
void serial_setbaudrate_57600(uint8_t port);
void serial_setbaudrate_76800(uint8_t port);
void serial_setbaudrate_115200(uint8_t port);
void serial_setbaudrate_230400(uint8_t port);

#endif