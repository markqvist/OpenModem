#include <stdbool.h>
#include <avr/io.h>

#include "device.h"
#include "hardware/VREF.h"
#include "hardware/AFSK.h"
#include "hardware/Serial.h"
#include "hardware/LED.h"
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

void system_check(void) {
    // TODO: Implement this
    LED_STATUS_ON();
}

void init(void) {
    sei();

    serial_init(&serial);    
    stdout = &serial.uart0;
    stdin  = &serial.uart0;

    VREF_init();
    LED_init();
    AFSK_init(&modem);
    ax25_init(&AX25, &modem, &modem.fd, ax25_callback);
    kiss_init(&AX25, &modem, &serial);

    system_check();
}

int main (void) {
    init();

    while (true) {
        ax25_poll(&AX25);
        kiss_poll();
        kiss_csma();
        led_status();
    }

    return(0);
}