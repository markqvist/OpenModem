#ifndef LED_H
#define LED_H

#include <avr/io.h>
#include "device.h"

void LED_init(void);
void LED_setIntensity(uint8_t value);

#define LED_STATUS_ON()   do { LED_PORT |= _BV(2); } while (0)
#define LED_STATUS_OFF()  do { LED_PORT &= ~_BV(2); } while (0)
#define LED_STATUS_TOGGLE()  do { LED_PORT ^= _BV(2); } while (0)
#define LED_TX_ON()   do { LED_PORT |= _BV(1); } while (0)
#define LED_TX_OFF()  do { LED_PORT &= ~_BV(1); } while (0)
#define LED_RX_ON()   do { LED_PORT |= _BV(0); } while (0)
#define LED_RX_OFF()  do { LED_PORT &= ~_BV(0); } while (0)

void LED_COM_ON(void);
void LED_COM_OFF(void);
void update_led_status(void);

#endif