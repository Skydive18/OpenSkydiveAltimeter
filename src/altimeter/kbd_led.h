#ifndef __in_keyboard_h
#define __in_keyboard_h
#include "Arduino.h"
#include "hwconfig.h"

#define BTN1_PRESSED (!(PORTC & 0x8))
#define BTN1_RELEASED !BTN1_PRESSED
#define BTN2_PRESSED (!(PORTC & 0x4))
#define BTN2_RELEASED !BTN2_PRESSED
#define BTN3_PRESSED (!(PORTC & 0x2))
#define BTN3_RELEASED !BTN2_PRESSED

uint8_t getKeypress(uint16_t timeout = 20000); // timeout in 15ms chunks, default is 5 min
void LED_show(byte red, byte green, byte blue, uint8_t delayMs = 0);

#endif
