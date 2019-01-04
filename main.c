#include <stdbool.h>
#include <avr/io.h>

#include "device.h"
#include "hardware/VREF.h"
#include "hardware/AFSK.h"
#include "hardware/Serial.h"
#include "protocol/AX25.h"
#include "protocol/KISS.h"
#include "util/time.h"
#include "util/FIFO.h"

Serial serial;
Afsk modem;
AX25Ctx AX25;

static void ax25_callback(struct AX25Ctx *ctx) {
    kiss_messageCallback(ctx);
}

void init(void) {
    sei();

    // TODO: serial init was last before
    serial_init(&serial);    
    stdout = &serial.uart0;
    stdin  = &serial.uart0;

    VREF_init();
    AFSK_init(&modem);
    ax25_init(&AX25, &modem, &modem.fd, ax25_callback);

    kiss_init(&AX25, &modem, &serial);
}

int main (void) {
    init();

    while (true) {
        ax25_poll(&AX25);
        
        if (serial_available(0)) {
            char sbyte = uart0_getchar_nowait();
            kiss_serialCallback(sbyte);
        }
    }

    return(0);
}