#ifndef __in_hwconfig_h
#define __in_hwconfig_h

// Features
#define LOGBOOK_ENABLE           /* Enables logbook */
#define SNAPSHOT_ENABLE          /* Enables jump trace recording. Requires LOGBOOK_ENABLE */
#define ALARM_ENABLE             /* Enables alarm clock. Requires sound system to be configured. */
//#define AUDIBLE_SIGNALS_ENABLE   /* TODO!! Enables audible altitude signals. Requires sound system to be configured. */
//#define TETRIS_ENABLE            /* TODO!! Enable 'tetris' game */
//#define SNAKE_ENABLE             /* TODO!! Enable 'snake' game */

// RTC epoch year, for PFC8583
#define EPOCH 2016

#define SERIAL_SPEED 57600

// Pin wiring

#if defined(__AVR_ATmega32U4__)
// Pins for Arduino Pro Micro (Atmega-32u4)
#define PLATFORM_1 'A'
#define PIN_HWPWR 1
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

#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
// Pins for Arduino Pro Mini (Atmega-328[P] - based)
#define PLATFORM_1 'B'
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

#else
#error CPU unsupported.
#endif // Platform

// Flash chip.
// Configure flash page size, depending on a flash chip used.
// FLASH page size is  32bytes for  24c32 ( 4K) and  24c64 ( 8K)
//                     64bytes for 24c128 (16K) and 24c256 (32K)
//                    128bytes for 24c512 (64K)
//#define FLASH_PRESENT
#define FLASH__PAGE_SIZE 32
#define FLASH__PAGES 128 /* 24c32 */
//#define FLASH__PAGES 256 /* 24c64, 24c128 */
//#define FLASH__PAGES 512 /* 24c256, 24c512 */
// I2C addfess for flash chip. Note, the default 24cxx address is 0x50 = the same as RTC chip uses.
// So flash chip should be put to a different i2c address by soldering its address lines accordingly.
#define FLASH_ADDRESS (uint8_t)0x51

// Jump snapshot journal configuration.
// For location, possible values EEPROM (= Controller's EEPROM) and FLASH (External flash chip).
// Max SNAPSHOT_SIZE seconds will be stored.
// Please note, SNAPSHOT_SIZE also acts as a bugbuf[] size and thus must not be less than approx. 400 bytes
#define SNAPSHOT_JOURNAL_LOCATION EEPROM
#define SNAPSHOT_JOURNAL_START 544
#define SNAPSHOT_JOURNAL_SIZE 1
#define SNAPSHOT_SIZE 480

// Logbook configuration.
// For location, possible values EEPROM and FLASH. Each logbook record is sizeof(jump_t) bytes (=12).
#define LOGBOOK_LOCATION EEPROM
#define LOGBOOK_START 64
#define LOGBOOK_SIZE 40

// EEPROM addressing
#define EEPROM_JUMP_COUNTER 0
#define EEPROM_SETTINGS 2

// Configure RGB LED. Define one of the following
#define LED_COMMON_CATHODE
//#define LED_COMMON_ANODE

// Configure display. Use one of the following
//#define DISPLAY_NOKIA
#define DISPLAY_HX1230

// Configure sound subsystem.
// Passive speaker/buzzer, connected to PIN_SOUND. Any frequency available.
//#define SOUND_PASSIVE

// Active buzzer with internal generator connected to PIN_SOUND. Only fixed tone available.
#define SOUND_ACTIVE

// External sound generator chip, i2c-based
//#define SOUND_EXTERNAL

// Checks

#if defined(SNAPSHOT_ENABLE) && !defined(LOGBOOK_ENABLE)
#error Jump snapshot feature requires Logbook feature to be enabled.
#endif

#if defined(DISPLAY_NOKIA)
#define PLATFORM_2 'N'
#elif defined(DISPLAY_HX1230)
#define PLATFORM_2 'H'
#else
#error Display not configured.
#endif

#if defined(FLASH_PRESENT)

#if FLASH__PAGE_SIZE==128 && FLASH__PAGES == 512
#define PLATFORM_3 '7' /* 24c512 */
#elif FLASH__PAGE_SIZE==64 && FLASH__PAGES == 512
#define PLATFORM_3 '6' /* 24c256 */
#elif FLASH__PAGE_SIZE==64 && FLASH__PAGES == 256
#define PLATFORM_3 '5' /* 24c128 */
#elif FLASH__PAGE_SIZE==32 && FLASH__PAGES == 256
#define PLATFORM_3 '4' /* 24c64 */
#elif FLASH__PAGE_SIZE==32 && FLASH__PAGES == 128
#define PLATFORM_3 '3' /* 24c32 */
#else
#define PLATFORM_3 'X'
#endif

#else
#define PLATFORM_3 '0'
#endif

#if defined (SOUND_PASSIVE)
#define PLATFORM_4 '1'
#define SOUND_USE_TIMER
#elif defined(SOUND_ACTIVE)
#define SOUND_USE_TIMER
#define PLATFORM_4 '2'
#elif defined(SOUND_EXTERNAL)
#define PLATFORM_4 '3'
#else
#define PLATFORM_4 '0'
#endif

#endif // __in_hwconfig_h
