#ifndef __in_keyboard_h
#define __in_keyboard_h
#include "Arduino.h"
#include "hwconfig.h"

#define BTN1_PRESSED (!(PORTF & 0x10))
#define BTN1_RELEASED !BTN1_PRESSED
#define BTN2_PRESSED (!(PORTD & 0x40))
#define BTN2_RELEASED !BTN2_PRESSED
#define BTN3_PRESSED (!(PORTF & 0x20))
#define BTN3_RELEASED !BTN3_PRESSED

uint8_t getKeypress(uint16_t timeout = 20000); // timeout in 15ms chunks, default is 5 min
#endif
