#include "Bluetooth.h"
#include <avr/pgmspace.h>

bool bluetooth_installed = false;

#define BT_AT_DELAY 100
void bluetooth_init(void) {
	BT_DDR &= ~(_BV(BT_MODE) | _BV(BT_RTS));
	BT_PORT |= _BV(BT_RTS);
	
	// Check module is in command mode
	if (BT_INPUT & _BV(BT_MODE)) {
		// Check module RTS pin is pulled low
		if (!(BT_INPUT &_BV(BT_RTS))) {
			// Reconfigure UART to 9600 baud and issue reset
			serial_setbaudrate_9600();
			delay_ms(BT_AT_DELAY*2);
			printf(PSTR("AT"));
			printf(PSTR("AT+FACTORYRESET\r\n"));
			delay_ms(BT_AT_DELAY);
			
			printf(PSTR("AT+GAPDEVNAME=OpenModem\r\n"));
			delay_ms(BT_AT_DELAY);

			printf(PSTR("AT+MODESWITCHEN=ble,0\r\n"));
			delay_ms(BT_AT_DELAY);

			printf(PSTR("AT+MODESWITCHEN=local,0\r\n"));
			delay_ms(BT_AT_DELAY);

			printf(PSTR("AT+HWMODELED=0\r\n"));
			delay_ms(BT_AT_DELAY);

			printf(PSTR("AT+BLEPOWERLEVEL=4\r\n"));
			delay_ms(BT_AT_DELAY);

			printf(PSTR("ATZ\r\n"));
			delay_ms(BT_AT_DELAY);
			
			// Reconfigure mode pin to output
			BT_DDR |= _BV(BT_MODE);

			// Enable DATA mode
			BT_PORT &= ~_BV(BT_MODE);

			bluetooth_installed = true;
		}
	}

	if (bluetooth_installed) {
		
	} else {
		
	}

}

bool bluetooth_enabled() {
	return bluetooth_installed;
}