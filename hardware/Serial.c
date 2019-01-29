#include "Serial.h"
#include <stdio.h>
#include <string.h>

extern volatile uint8_t queue_height;

void serial_init(Serial *serial) {
    memset(serial, 0, sizeof(*serial));
    memset(serialBuf, 0, sizeof(serialBuf));

    serial_setbaudrate_115200();

    // Set to 8-bit data, enable RX and TX, enable receive interrupt
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

    FILE uart0_fd = FDEV_SETUP_STREAM(uart0_putchar, uart0_getchar, _FDEV_SETUP_RW);
    
    serial->uart0 = uart0_fd;

    fifo_init(&serialFIFO, serialBuf, sizeof(serialBuf));
}

bool serial_available(uint8_t index) {
    if (index == 0) {
        if (UCSR0A & _BV(RXC0)) return true;
    }
    return false;
}


int uart0_putchar(char c, FILE *stream) {
    LED_COM_ON();
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    return 1;
}

int uart0_getchar(FILE *stream) {
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}

char uart0_getchar_nowait(void) {
    if (!(UCSR0A & _BV(RXC0))) return EOF;
    return UDR0;
}

ISR(USART0_RX_vect) {
    if (serial_available(0)) {
        LED_COM_ON();
        if (!fifo_isfull(&serialFIFO)) {
            char c = uart0_getchar_nowait();
            fifo_push(&serialFIFO, c);
        } else {
            // TODO: Remove this
            printf("SERIAL FIFO OVERRUN\r\n");
            printf("QH: %d", queue_height);
            while(true) {
                LED_TX_ON();
                LED_RX_ON();
                LED_COM_ON();
            }
        }
    }
}

void serial_setbaudrate_1200(void) {
    #undef BAUD
    #define BAUD 1200
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_2400(void) {
    #undef BAUD
    #define BAUD 2400
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_4800(void) {
    #undef BAUD
    #define BAUD 4800
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}



void serial_setbaudrate_9600(void) {
    #undef BAUD
    #define BAUD 9600
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_14400(void) {
    #undef BAUD
    #define BAUD 14400
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_19200(void) {
    #undef BAUD
    #define BAUD 19200
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_28800(void) {
    #undef BAUD
    #define BAUD 28800
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_38400(void) {
    #undef BAUD
    #define BAUD 38400
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_57600(void) {
    #undef BAUD
    #define BAUD 57600
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_76800(void) {
    #undef BAUD
    #define BAUD 76800
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_115200(void) {
    #undef BAUD
    #define BAUD 115200
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

void serial_setbaudrate_230400(void) {
    #undef BAUD
    #define BAUD 230400
    #include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
    #if USE_2X
        UCSR0A |= _BV(U2X0);
    #else
        UCSR0A &= ~(_BV(U2X0));
    #endif
}

