#include "LED.h"
#include "util/time.h"

uint8_t ledIntensity = CONFIG_LED_INTENSITY;
uint32_t com_led_timeout = 0;

void LED_init(void) {
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
    int32_t xa = timer_clock();
    com_led_timeout = xa + ms_to_ticks(COM_LED_TIMEOUT_MS);
    if (xa > com_led_timeout) {
        while(true) {
            LED_COM_ON();
            LED_RX_ON();
            LED_TX_ON();
        }
    }
}

void LED_COM_OFF(void) {
    LED_PORT &= ~_BV(4); 
}

void led_status(void) {
    if (timer_clock() > com_led_timeout) {
        if (LED_PORT & _BV(4)) {
            printf("%d\r\n", timer_clock());
            LED_COM_OFF();
        }
    }
}