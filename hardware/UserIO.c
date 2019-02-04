#include "UserIO.h"
#include "hardware/Bluetooth.h"

void usrio_init(void) {
	USR_IO_DDR |= _BV(USR_IO_1) | _BV(USR_IO_2);
	if (!bluetooth_enabled()) {
		USR_IO_DDR |= _BV(USR_IO_3) | _BV(USR_IO_4);
	}
}

bool usrio_1(void) {
	if (USR_IO_PORT & _BV(USR_IO_1)) {
		return true;
	} else {
		return false;
	}
}

bool usrio_2(void) {
	if (USR_IO_PORT & _BV(USR_IO_2)) {
		return true;
	} else {
		return false;
	}
}

bool usrio_3(void) {
	if (!bluetooth_enabled()) {
		if (USR_IO_PORT & _BV(USR_IO_3)) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

bool usrio_4(void) {
	if (!bluetooth_enabled()) {
		if (USR_IO_PORT & _BV(USR_IO_4)) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

void usrio_1_on(void) {
	USR_IO_PORT |= _BV(USR_IO_1);
}

void usrio_2_on(void) {
	USR_IO_PORT |= _BV(USR_IO_2);
}

void usrio_3_on(void) {
	if (!bluetooth_enabled()) { USR_IO_PORT |= _BV(USR_IO_3); }
}

void usrio_4_on(void) {
	if (!bluetooth_enabled()) { USR_IO_PORT |= _BV(USR_IO_4); }
}

void usrio_1_off(void) {
	USR_IO_PORT &= ~_BV(USR_IO_1);
}

void usrio_2_off(void) {
	USR_IO_PORT &= ~_BV(USR_IO_2);
}

void usrio_3_off(void) {
	if (!bluetooth_enabled()) { USR_IO_PORT |= _BV(USR_IO_3); }
}

void usrio_4_off(void) {
	if (!bluetooth_enabled()) { USR_IO_PORT |= _BV(USR_IO_4); }
}

void usrio_1_toggle(void) {
	if (!bluetooth_enabled()) {
		if (usrio_1()) {
			usrio_1_off();
		} else {
			usrio_1_on();
		}
	}
}

void usrio_2_toggle(void) {
	if (!bluetooth_enabled()) {
		if (usrio_2()) {
			usrio_2_off();
		} else {
			usrio_2_on();
		}
	}
}

void usrio_3_toggle(void) {
	if (usrio_3()) {
		usrio_3_off();
	} else {
		usrio_3_on();
	}
}

void usrio_4_toggle(void) {
	if (usrio_4()) {
		usrio_4_off();
	} else {
		usrio_4_on();
	}
}

// TODO: Add ADC read support to UserIO