#ifndef __in_keyboard_h
#define __in_keyboard_h
#include "Arduino.h"
#include "hwconfig.h"

#define BTN1_PRESSED ((PINC & 0x8) == 0)
#define BTN1_RELEASED !BTN1_PRESSED
#define BTN2_PRESSED ((PINC & 0x4) == 0)
#define BTN2_RELEASED !BTN2_PRESSED
#define BTN3_PRESSED ((PINC & 0x2) == 0)
#define BTN3_RELEASED !BTN3_PRESSED

uint8_t getKeypress(bool enable_repeat = false); // timeout in 15ms chunks, default is 5 min
void LED_show(byte red, byte green, byte blue, uint8_t delayMs = 0);

#endif
