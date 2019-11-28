#ifndef __in_hwconfig_inc
#define __in_hwconfig_inc

#define EPOCH 2016

#define SERIAL_SPEED 57600

#if defined(__AVR_ATmega32U4__)
// Pins for Arduino Pro Micro (Atmega-32u4)
#define PIN_HWPWR 8
#define PIN_LIGHT 4
#define PIN_R 5
#define PIN_G 9
#define PIN_B 10
#define PIN_BTN1 A3
#define PIN_BTN2 0
#define PIN_BTN3 A1
#define PIN_BAT_SENSE A0
#define PIN_SOUND 6
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

// Flash chip
// FLASH page size is  32bytes for  24c32 and  24c64,
//                      64bytes for 24c128 and 24c256
//                     128bytes for 24c512
#define FLASH_ENABLE
#define FLASH__SIZE_KB 4
#define FLASH__PAGE_SIZE 32
#define FLASH_ADDRESS (uint8_t)0x51

// Jump snapshot configuration.
// For location, possible values EEPROM (= Controller's EEPROM), FLASH (External flash card).
// Max SNAPSHOT_SIZE seconds will be stored. Compression level is 5/8

#define SNAPSHOT_ENABLE
#define SNAPSHOT_JOURNAL_LOCATION FLASH
/*

#define SNAPSHOT_JOURNAL_START 544
#define SNAPSHOT_JOURNAL_SIZE 1
#define SNAPSHOT_SIZE 480
#define SNAPSHOT_COMPRESSED_SIZE 1+ (SNAPSHOT_SIZE*5/8)

// Logbook configuration.
// For location, possible values EEPROM and FLASH
#define LOGBOOK_ENABLE
#define LOGBOOK_LOCATION EEPROM
#define LOGBOOK_SIZE 40

*/

// EEPROM
#define EEPROM_JUMP_COUNTER 0
#define EEPROM_SETTINGS 2
#define EEPROM_LOGBOOK_START 64
// FLASH page size is  32bytes for  24c32 and  24c64,
//                      64bytes for 24c128 and 24c256
//                     128bytes for 24c512
#define FLASH__PAGE_SIZE         32

// Configure RGB LED. Define one of the following
#define LED_COMMON_CATHODE
//#define LED_COMMON_ANODE

// Configure display. Use one of the following
#define DISPLAY_NOKIA
//#define DISPLAY_HX1230


// Configure sound subsystem.
// Passive speaker/buzzer, connected to PIN_SOUND. Any frequency available.
//#define SOUND_PASSIVE

// Active buzzer with internal generator connected to PIN_SOUND. Only fixed tone available.
#define SOUND_ACTIVE

// External sound generator chip, i2c-based
//#define SOUND_EXTERNAL

#endif
