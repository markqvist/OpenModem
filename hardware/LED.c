#include "LED.h"
#include "util/time.h"

uint8_t ledIntensity = CONFIG_LED_INTENSITY;
ticks_t led_status_ticks_top = 0;
ticks_t led_status_ticks = 0;
ticks_t com_led_timeout = 0;

void LED_init(void) {
    led_status_ticks_top = ms_to_ticks(CONFIG_LED_UPDATE_INTERVAL_MS);

    // Enable output for LED pins and drain pin
    LED_DDR |= _BV(0) |	// RX
    		   _BV(1) | // TX
    		   _BV(2) | // Status
    		   _BV(3) | // PWM drain
               _BV(4);  // COM

    LED_PORT &= 0b11100000;

    TCCR0A = _BV(WGM00) |
             _BV(WGM01) |
             _BV(COM0A1)|
             _BV(COM0A0);   

    TCCR0B = _BV(CS00);

    OCR0A = ledIntensity;
}


void LED_setIntensity(uint8_t value) {
	ledIntensity = value;
	OCR0A = ledIntensity;
}

void LED_COM_ON(void) {
    LED_PORT |= _BV(4);
    com_led_timeout = timer_clock() + ms_to_ticks(CONFIG_COM_LED_TIMEOUT_MS);
}

void LED_COM_OFF(void) {
    LED_PORT &= ~_BV(4); 
}

void update_led_status(void) {
    if (led_status_ticks >= led_status_ticks_top) {
        if (timer_clock() > com_led_timeout) {
            LED_COM_OFF();
        }
        led_status_ticks = 0;
    } else {
        led_status_ticks++;
    }
}

void LED_indicate_error_crypto(void) {
    while (true) {
        LED_COM_ON();
        LED_STATUS_OFF();
        delay_ms(500);
        LED_COM_OFF();
        LED_STATUS_ON();
        delay_ms(500);
    }
}