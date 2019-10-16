#ifndef __in_hwconfig_inc
#define __in_hwconfig_inc

#define EPOCH 2016

#define SERIAL_SPEED 57600

#if defined(__AVR_ATmega32U4__)
// Pins for Arduino Pro Micro (Atmega-32u4)
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
#define PIN_DC 30
//#define LOGBOOK_SIZE 80
// TODO!!
#define LOGBOOK_SIZE 20
#define SNAPSHOT_START 304
#define SNAPSHOT_SIZE 720
#else
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
// Pins for Arduino Pro Mini (Atmega-328[P] - based)
#define PIN_HWPWR 4
#define PIN_LIGHT 7
#define PIN_R 5
#define PIN_G 6
#define PIN_B 9
#define PIN_BTN1 A3
#define PIN_BTN2 A2
#define PIN_BTN3 A1
#define PIN_BAT_SENSE A0
#define PIN_SOUND 3
#define PIN_INTERRUPT 2
#define PIN_DC 8
#define LOGBOOK_SIZE 40
#define SNAPSHOT_START 544
#define SNAPSHOT_SIZE 480
#endif // defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#endif // defined(__AVR_ATmega32U4__)

// EEPROM
#define EEPROM_JUMP_COUNTER 0
#define EEPROM_LOGBOOK_START 64

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

#endif
