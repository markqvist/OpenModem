#ifndef LED_H
#define LED_H

#include <avr/io.h>
#include <stdbool.h>
#include "device.h"

extern bool LED_softblock_enabled;

void LED_init(void);
void LED_setIntensity(uint8_t value);

#define LED_STATUS_ON()   do { if (!LED_softblock_enabled) LED_PORT |= _BV(2); } while (0)
#define LED_STATUS_OFF()  do { if (!LED_softblock_enabled) LED_PORT &= ~_BV(2); } while (0)
#define LED_STATUS_TOGGLE()  do { if (!LED_softblock_enabled) LED_PORT ^= _BV(2); } while (0)
#define LED_TX_ON()   do { if (!LED_softblock_enabled) LED_PORT |= _BV(1); } while (0)
#define LED_TX_OFF()  do { if (!LED_softblock_enabled) LED_PORT &= ~_BV(1); } while (0)
#define LED_RX_ON()   do { if (!LED_softblock_enabled) LED_PORT |= _BV(0); } while (0)
#define LED_RX_OFF()  do { if (!LED_softblock_enabled) LED_PORT &= ~_BV(0); } while (0)

#define LED_F_STATUS_ON()   do { LED_PORT |= _BV(2); } while (0)
#define LED_F_STATUS_OFF()  do { LED_PORT &= ~_BV(2); } while (0)
#define LED_F_STATUS_TOGGLE()  do { LED_PORT ^= _BV(2); } while (0)
#define LED_F_TX_ON()   do { LED_PORT |= _BV(1); } while (0)
#define LED_F_TX_OFF()  do { LED_PORT &= ~_BV(1); } while (0)
#define LED_F_RX_ON()   do { LED_PORT |= _BV(0); } while (0)
#define LED_F_RX_OFF()  do { LED_PORT &= ~_BV(0); } while (0)

void LED_COM_ON(void);
void LED_COM_OFF(void);
void update_led_status(void);

void LED_indicate_error_crypto(void);

void LED_indicate_enabled_crypto(void);

#endif