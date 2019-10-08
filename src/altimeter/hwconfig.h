#ifndef __in_hwconfig_inc
#define __in_hwconfig_inc

#define EPOCH 2016

// Pins for Arduino Pro Micro
#define PIN_HWPWR 8
#define PIN_LIGHT 4
#define PIN_R 6
#define PIN_G 9
#define PIN_B 10
#define PIN_BTN1 A3
#define PIN_BTN2 A2
#define PIN_BTN3 A1
#define PIN_BAT_SENSE A0
#define PIN_SOUND 5
#define PIN_INTERRUPT 7

// Address of LED profiles in NVRAM
#define LED_PROFILES_START 100
// Address of logbook in NVRAM
#define LOGBOOK_START 124

// Configure RGB LED. Define one of the following
#define LED_COMMON_CATHODE
//#define LED_COMMON_ANODE


// Configure display. Use one of the following
#define DISPLAY_NOKIA
//#define DISPLAY_HX1230
// Use these macros if display light is turned on with logic 0
#define DISPLAY_LIGHT_OFF digitalWrite(PIN_LIGHT,1)
#define DISPLAY_LIGHT_ON digitalWrite(PIN_LIGHT,0)
// Use these macros if display light is turned on with logic 1
//#define DISPLAY_LIGHT_OFF digitalWrite(PIN_LIGHT,0)
//#define DISPLAY_LIGHT_ON digitalWrite(PIN_LIGHT,1)

// Configure RTC. Refer to i2c scan to determine correct address
#include "PCF8583.h"

#endif
