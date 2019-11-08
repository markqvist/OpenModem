#include <stdbool.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "device.h"
#include "hardware/VREF.h"
#include "hardware/AFSK.h"
#include "hardware/Serial.h"
#include "hardware/LED.h"
#include "hardware/UserIO.h"
#include "hardware/SD.h"
#include "hardware/Crypto.h"
#include "hardware/Bluetooth.h"
#include "hardware/GPS.h"
#include "protocol/AX25.h"
#include "protocol/KISS.h"
#include "util/Config.h"
#include "util/time.h"
#include "util/FIFO.h"

uint8_t boot_vector = 0x00;
uint8_t OPTIBOOT_MCUSR __attribute__ ((section(".noinit")));
void resetFlagsInit(void) __attribute__ ((naked)) __attribute__ ((used)) __attribute__ ((section (".init0")));
void resetFlagsInit(void) {
    __asm__ __volatile__ ("sts %0, r2\n" : "=m" (OPTIBOOT_MCUSR) :);
}

Serial serial;
Afsk modem;
AX25Ctx AX25;

static void ax25_callback(struct AX25Ctx *ctx) {
    kiss_messageCallback(ctx);
}

void system_check(void) {
    // Check boot vector
    if (OPTIBOOT_MCUSR & (1<<PORF)) {
      boot_vector = START_FROM_POWERON;
    } else if (OPTIBOOT_MCUSR & (1<<BORF)) {
      boot_vector = START_FROM_BROWNOUT;
    } else if (OPTIBOOT_MCUSR & (1<<WDRF)) {
      boot_vector = START_FROM_BOOTLOADER;
    } else {
        printf(PSTR("Error, indeterminate boot vector %d\r\n"), OPTIBOOT_MCUSR);
        printf(PSTR("System start has been halted\r\n"));
        while (true) {
            LED_TX_ON();
            LED_COM_ON();
        }
    }

    // If encryption was previously enabled, require
    // it to be initialised to start system.
    if (config_crypto_lock) {
        if (!crypto_wait()) {
            // If initialising crypto times out,
            // halt system and display error signal
            LED_indicate_error_crypto();
        }
    }

    // TODO: Check GPS_REQUIRED and BLUETOOTH_REQUIRED
    // here before giving green light.

    // Give the green light if everything checks out
    LED_STATUS_ON();
}

void init(void) {

    sei();

    serial_init(&serial);    
    stdout = &serial.uart0;
    stdin  = &serial.uart0;

    config_init();

    VREF_init();
    LED_init();
    AFSK_init(&modem);
    ax25_init(&AX25, &modem, &modem.fd, ax25_callback);
    kiss_init(&AX25, &modem, &serial);
    sd_init();
    bluetooth_init();
    gps_init(&serial);
    usrio_init();

    system_check();
}


int main (void) {
    init();

    while (true) {
        ax25_poll(&AX25);
        kiss_poll();
        kiss_csma();
        sd_jobs();
        gps_poll();
    }

    return(0);
}