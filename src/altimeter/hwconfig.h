#ifndef __in_hwconfig_h
#define __in_hwconfig_h

// ******************
// Hardware constants

// RTC type constants
#define RTC_PCF8583 1
#define RTC_PCF8563 2

// Display type constants
#define DISPLAY_NOKIA5110 1
#define DISPLAY_NOKIA1201 2
#define DISPLAY_TM9665ACC 3

// Pressure sensor type constants
#define SENSOR_MPL3115A2 1
#define SENSOR_BMP180 2
#define SENSOR_BMP280 3
#define SENSOR_BME388 4

// Sound system constants
#define SOUND_NONE 0
#define SOUND_PASSIVE 1
#define SOUND_ACTIVE 2
#define SOUND_VOLUME_CONTROL_ADDR 0x2f

// Data location constants
#define LOCATION_EEPROM 1
#define LOCATION_FLASH 2

// Serial port speed
#define SERIAL_SPEED 9600L

// Language constants
#define LANGUAGE_ENGLISH 0
#define LANGUAGE_RUSSIAN 1

// Software Features
#define LOGBOOK_ENABLE           /* Enables logbook */
#define SNAPSHOT_ENABLE          /* Enables jump trace recording. Requires LOGBOOK_ENABLE */
//#define ALARM_ENABLE             /* Enables alarm clock. Requires sound system to be configured. */
//#define AUDIBLE_SIGNALS_ENABLE   /* Enables audible altitude signals. Requires sound system to be configured. */
//#define TETRIS_ENABLE            /* TODO!! Enable 'tetris' game */
//#define SNAKE_ENABLE             /* TODO!! Enable 'snake' game */
#define LANGUAGE LANGUAGE_RUSSIAN

// Configure RTC chip
#define RTC RTC_PCF8563

// Configure RGB LED. Define one of the following
#define LED_COMMON_CATHODE
//#define LED_COMMON_ANODE

// Configure display. Use one of the following
#define DISPLAY DISPLAY_NOKIA1201

// Configure sound subsystem.
#define SOUND SOUND_PASSIVE
//#define SOUND_VOLUME_CONTROL_ENABLE
// Flash chip.
// Configure flash page size, depending on a flash chip used.
// FLASH page size is  32bytes for  24c32 ( 4K) and  24c64 ( 8K)
//                     64bytes for 24c128 (16K) and 24c256 (32K)
//                    128bytes for 24c512 (64K)
//#define FLASH_PRESENT
#define FLASH__PAGE_SIZE 128
//#define FLASH__PAGES 128 /* 24c32 */
//#define FLASH__PAGES 256 /* 24c64, 24c128 */
#define FLASH__PAGES 512 /* 24c256, 24c512 */
// I2C addfess for flash chip. Note, the default 24cxx address is 0x50 = the same as PCF8583 uses.
// Also note, the address of PCF8563 is 0x51.
// So flash chip should be put to a different i2c address by soldering its address lines accordingly.
#if RTC==RTC_PCF8583
#define FLASH_ADDRESS (uint8_t)0x51
#elif RTC==RTC_PCF8563
#define FLASH_ADDRESS (uint8_t)0x53
#else
#error RTC chip should be defined before flash configuration!
#endif

// EEPROM addressing
#define EEPROM_JUMP_COUNTER 0x0
#define EEPROM_SETTINGS 0x2
#define EEPROM_VOLUMEMAP 0x1c
#define EEPROM_JUMP_PROFILES 0x20
#define EEPROM_AUDIBLE_SIGNALS 0x30
#define EEPROM_FREE_AREA 0x130

// Logbook and Jump snapshot journal configuration.
// For location, possible values LOCATION_EEPROM (= Controller's EEPROM) and LOCATION_FLASH (External flash chip).
// Max SNAPSHOT_SIZE seconds will be stored.
// Please note, SNAPSHOT_SIZE also acts as a bugbuf[] size and thus must not be less than approx. 400 bytes
// One logbook record length is sizeof(jump_t) bytes (=12).

// Predefined offered configurations.

/*
// 1. No external flash. 25 jumps, 1 snapshot
#define SNAPSHOT_JOURNAL_LOCATION LOCATION_EEPROM
#define SNAPSHOT_JOURNAL_START 0x25c
#define SNAPSHOT_JOURNAL_SIZE 1
#define SNAPSHOT_SIZE 420
#define LOGBOOK_LOCATION LOCATION_EEPROM
#define LOGBOOK_START EEPROM_FREE_AREA
#define LOGBOOK_SIZE 25
*/

/*
// 2. Flash 4kb. 120 jumps, 6 snapshots, all in flash
#define SNAPSHOT_JOURNAL_LOCATION LOCATION_FLASH
#define SNAPSHOT_JOURNAL_START 1440
#define SNAPSHOT_JOURNAL_SIZE 6
#define SNAPSHOT_SIZE 420
#define LOGBOOK_LOCATION LOCATION_FLASH
#define LOGBOOK_START 0
#define LOGBOOK_SIZE 120
#define FLASH_PRESENT
*/

/*
// 3. Flash 4kb. 60 jumps in eeprom, 9 snapshots in flash
#define SNAPSHOT_JOURNAL_LOCATION LOCATION_FLASH
#define SNAPSHOT_JOURNAL_START 0
#define SNAPSHOT_JOURNAL_SIZE 9
#define SNAPSHOT_SIZE 420
#define LOGBOOK_LOCATION LOCATION_EEPROM
#define LOGBOOK_START EEPROM_FREE_AREA
#define LOGBOOK_SIZE 60
#define FLASH_PRESENT
*/


// 3. Flash 64kb. 1000 jumps in eeprom, 120 snapshots in eeprom
#define SNAPSHOT_JOURNAL_LOCATION LOCATION_FLASH
#define SNAPSHOT_JOURNAL_START 12000
#define SNAPSHOT_JOURNAL_SIZE 120
#define SNAPSHOT_SIZE 420
#define LOGBOOK_LOCATION LOCATION_FLASH
#define LOGBOOK_START 0
#define LOGBOOK_SIZE 1000
#define FLASH_PRESENT

// Pin wiring

#if defined(__AVR_ATmega32U4__)
// Pins for Arduino Pro Micro (Atmega-32u4)
#if RTC==RTC_PCF8583
#define PLATFORM_1 'A'
#elif RTC==RTC_PCF8563
#define PLATFORM_1 'C'
#else
#define PLATFORM_1 'Z'
#endif
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
#define PIN_INTERRUPT_HEARTBEAT 8
#define PIN_DC 30

#elif defined(__AVR_ATmega328P__)
// Pins for Arduino Pro Mini (Atmega-328[P] - based)
#if RTC==RTC_PCF8583
#define PLATFORM_1 'B'
#elif RTC==RTC_PCF8563
#define PLATFORM_1 'D'
#else
#define PLATFORM_1 'Y'
#endif
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
#define PIN_INTERRUPT_HEARTBEAT 2
#define PIN_DC 8

#else
#error CPU unsupported.
#endif // Platform

// Checks

#if defined(SNAPSHOT_ENABLE) && !defined(LOGBOOK_ENABLE)
#error Jump snapshot feature requires Logbook feature to be enabled.
#endif

#if RTC==RTC_PCF8583
#define WIRE_SPEED 100000
#elif RTC==RTC_PCF8563
#define WIRE_SPEED 400000
#endif

#if DISPLAY==DISPLAY_NOKIA5110
#define PLATFORM_2 'N'
#elif DISPLAY==DISPLAY_NOKIA1201
#define PLATFORM_2 'H'
#elif DISPLAY==DISPLAY_TM9665ACC
#define PLATFORM_2 'T'
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

#if SOUND==SOUND_PASSIVE
#define PLATFORM_4 '1'
#define SOUND_USE_TIMER
#elif SOUND==SOUND_ACTIVE
#define SOUND_USE_TIMER
#define PLATFORM_4 '2'
#else
#define PLATFORM_4 '0'
#endif

#if defined(SOUND_VOLUME_CONTROL_ENABLE) && SOUND != SOUND_PASSIVE
#undef SOUND_VOLUME_CONTROL_ENABLE
#endif

#endif // __in_hwconfig_h
