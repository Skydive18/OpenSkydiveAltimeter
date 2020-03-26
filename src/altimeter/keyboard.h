#ifndef __in_keyboard_h
#define __in_keyboard_h
#include "Arduino.h"
#include "hwconfig.h"

#define BTN1_PRESSED (!(PINF & 0x10))
#define BTN1_RELEASED !BTN1_PRESSED
#define BTN2_PRESSED (!(PINF & 0x40))
#define BTN2_RELEASED !BTN2_PRESSED
#define BTN3_PRESSED (!(PINF & 0x20))
#define BTN3_RELEASED !BTN3_PRESSED

uint8_t getKeypress(bool enable_repeat = false);
#endif
