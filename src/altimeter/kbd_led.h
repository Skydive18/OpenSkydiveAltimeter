#ifndef __in_keyboard_h
#define __in_keyboard_h
#include "Arduino.h"
#include "hwconfig.h"

#define BTN1_PRESSED !digitalRead(PIN_BTN1)
#define BTN1_RELEASED digitalRead(PIN_BTN1)
#define BTN2_PRESSED !digitalRead(PIN_BTN2)
#define BTN2_RELEASED digitalRead(PIN_BTN2)
#define BTN3_PRESSED !digitalRead(PIN_BTN3)
#define BTN3_RELEASED digitalRead(PIN_BTN3)

uint8_t getKeypress(uint16_t timeout = 20000); // timeout in 15ms chunks, default is 5 min
void LED_show(byte red, byte green, byte blue, uint8_t delayMs = 0);

#endif