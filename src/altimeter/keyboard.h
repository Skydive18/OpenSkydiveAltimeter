#ifndef __in_keyboard_h
#define __in_keyboard_h
#include "hwconfig.h"

#define BTN1_PRESSED !digitalRead(PIN_BTN1)
#define BTN1_RELEASED digitalRead(PIN_BTN1)
#define BTN2_PRESSED !digitalRead(PIN_BTN2)
#define BTN2_RELEASED digitalRead(PIN_BTN2)
#define BTN3_PRESSED !digitalRead(PIN_BTN3)
#define BTN3_RELEASED digitalRead(PIN_BTN3)

#endif
