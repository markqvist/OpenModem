#include "Serial.h"
#include <stdio.h>
#include <string.h>
#include "util/Config.h"

extern volatile uint8_t queue_height;

void serial_init(Serial *serial) {
    memset(serial, 0, sizeof(*serial));
    memset(uart0Buf, 0, sizeof(uart0Buf));
    memset(uart1Buf, 0, sizeof(uart1Buf));
    
    serial_setbaudrate_115200(0);
    serial_setbaudrate_115200(1);

    // Set to 8-bit data, enable RX and TX, enable receive interrupt
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
    UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);

    FILE uart0_fd = FDEV_SETUP_STREAM(uart0_putchar, uart0_getchar, _FDEV_SETUP_RW);
    FILE uart1_fd = FDEV_SETUP_STREAM(uart1_putchar, uart1_getchar, _FDEV_SETUP_RW);
    
    serial->uart0 = uart0_fd;
    serial->uart1 = uart1_fd;

    fifo_init(&uart0FIFO, uart0Buf, CONFIG_UART0_BUFFER_SIZE);
    fifo_init(&uart1FIFO, uart1Buf, CONFIG_UART1_BUFFER_SIZE);

}

bool serial_available(uint8_t index) {
    if (index == 0) {
        if (UCSR0A & _BV(RXC0)) return true;
    } else if (index == 1) {
        if (UCSR1A & _BV(RXC1)) return true;
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
    LED_COM_ON();
    return UDR0;
}

char uart0_getchar_nowait(void) {
    if (!(UCSR0A & _BV(RXC0))) return EOF;
    LED_COM_ON();
    return UDR0;
}

int uart1_putchar(char c, FILE *stream) {
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = c;
    return 1;
}

int uart1_getchar(FILE *stream) {
    loop_until_bit_is_set(UCSR1A, RXC1);
    return UDR1;
}

char uart1_getchar_nowait(void) {
    if (!(UCSR1A & _BV(RXC1))) return EOF;
    return UDR1;
}

ISR(USART0_RX_vect) {
    if (serial_available(0)) {
        LED_COM_ON();
        if (!fifo_isfull(&uart0FIFO)) {
            char c = uart0_getchar_nowait();
            fifo_push(&uart0FIFO, c);
        } else {
            //uart0_getchar_nowait();
        }
    }
}

ISR(USART1_RX_vect) {
    if (serial_available(1)) {
        if (!fifo_isfull(&uart1FIFO)) {
            char c = uart1_getchar_nowait();
            fifo_push(&uart1FIFO, c);
        } else {
            //uart1_getchar_nowait();
        }
    }
}

void serial_setbaudrate_1200(uint8_t port) {
    #undef BAUD
    #define BAUD 1200
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_2400(uint8_t port) {
    #undef BAUD
    #define BAUD 2400
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_4800(uint8_t port) {
    #undef BAUD
    #define BAUD 4800
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}



void serial_setbaudrate_9600(uint8_t port) {
    #undef BAUD
    #define BAUD 9600
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_14400(uint8_t port) {
    #undef BAUD
    #define BAUD 14400
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_19200(uint8_t port) {
    #undef BAUD
    #define BAUD 19200
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_28800(uint8_t port) {
    #undef BAUD
    #define BAUD 28800
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_38400(uint8_t port) {
    #undef BAUD
    #define BAUD 38400
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_57600(uint8_t port) {
    #undef BAUD
    #define BAUD 57600
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_76800(uint8_t port) {
    #undef BAUD
    #define BAUD 76800
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_115200(uint8_t port) {
    #undef BAUD
    #define BAUD 115200
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

void serial_setbaudrate_230400(uint8_t port) {
    #undef BAUD
    #define BAUD 230400
    #include <util/setbaud.h>
    if (port == 0) {
        UBRR0H = UBRRH_VALUE; UBRR0L = UBRRL_VALUE;
        #if USE_2X
            UCSR0A |= _BV(U2X0);
        #else
            UCSR0A &= ~(_BV(U2X0));
        #endif
    } else if (port == 1) {
        UBRR1H = UBRRH_VALUE; UBRR1L = UBRRL_VALUE;
        #if USE_2X
            UCSR1A |= _BV(U2X0);
        #else
            UCSR1A &= ~(_BV(U2X0));
        #endif
    }
}

