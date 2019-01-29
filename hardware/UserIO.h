#ifndef USRIO_H
#define USRIO_H

#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include "device.h"

void usrio_init(void);

bool usrio_1(void);
bool usrio_2(void);
bool usrio_3(void);
bool usrio_4(void);

void usrio_1_on(void);
void usrio_2_on(void);
void usrio_3_on(void);
void usrio_4_on(void);

void usrio_1_off(void);
void usrio_2_off(void);
void usrio_3_off(void);
void usrio_4_off(void);

void usrio_1_toggle(void);
void usrio_2_toggle(void);
void usrio_3_toggle(void);
void usrio_4_toggle(void);

#endif