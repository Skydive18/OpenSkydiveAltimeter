#include <EEPROM.h>
#include "power.h"
#include <SPI.h>
#include <Wire.h>
#include "MPL.h"
#include "hwconfig.h"
#include "led.h"
#include "keyboard.h"
#include "display.h"
#if DISPLAY==DISPLAY_NOKIA5110
#include "fonts/fonts_nokia.h"
#endif
#if DISPLAY==DISPLAY_NOKIA1201
#include "fonts/fonts_hx1230.h"
#endif
#include "custom_types.h"
#include "common.h"
#include "RTC.h"
#include "snd.h"
#include "logbook.h"

#ifdef FLASH_PRESENT
#include "flash.h"
extern FlashRom flashRom;
#endif

#if LANGUAGE==LANGUAGE_RUSSIAN
#include "i18n/messages_ru.h"
#elif LANGUAGE==LANGUAGE_ENGLISH
#include "i18n/messages_en.h"
#else
#error Unknown language, only English and Russian supported.
#endif

// Altimeter modes (powerMode)
#define MODE_ON_EARTH 0
#define MODE_IN_AIRPLANE 1
#define MODE_EXIT 2
#define MODE_BEGIN_FREEFALL 3
#define MODE_FREEFALL 4
#define MODE_PULLOUT 5
#define MODE_OPENING 6
#define MODE_UNDER_PARACHUTE 7
#define MODE_PREFILL 8
#define MODE_DUMB 9
//
// State machine sequences
//
//   Dumb) DUMB -> DUMB (Show altitude only; no automatic, no logbook, etc.)
// Normal) PREFILL -> ON_EARTH <-> IN_AIRPLANE <-> EXIT <-> BEGIN_FREEFALL -> FREEFALL <-> PULLOUT <-> OPENING -> UNDER_PARACHUTE -> ON_EARTH
//
// State machine points
//
#define EXIT_TH -jump_profile.exit
#define BEGIN_FREEFALL_TH -jump_profile.begin_freefall
#define FREEFALL_TH -jump_profile.freefall
#define PULLOUT_TH -jump_profile.pullout
#define OPENING_TH -jump_profile.opening
#define UNDER_PARACHUTE_TH -jump_profile.under_parachute

settings_t settings;
jump_profile_t jump_profile;
#ifdef AUDIBLE_SIGNALS_ENABLE
audible_signals_t audible_signals;
#endif

MPL3115A2 myPressure;
Rtc rtc;
void(* resetFunc) (void) = 0;

#define CLEAR_PREVIOUS_ALTITUDE -30000
int vspeed[32]; // used in mode detector. TODO clean it in setup maybe?
uint32_t interval_number; // global heartbeat counter
long heartbeat; // heartbeat in 500ms ticks until auto turn off
uint8_t vspeedPtr = 0; // pointer to write to vspeed buffer
int current_speed;
int average_speed_8;
int average_speed_32;
// Auto zero drift correction
uint8_t zero_drift_sense = 128;


// ********* Main power move flag
uint8_t powerMode;
uint8_t previousPowerMode;
volatile bool INTACK;

int current_altitude; // currently measured altitude, relative to ground_altitude
int previous_altitude; // Previously measured altitude, relative to ground_altitude
int altitude_to_show; // Last shown altitude (jitter corrected)
uint16_t batt; // battery voltage as it goes from sensor
int8_t rel_voltage; // ballery relative voltage, in percent (0-100). 0 relates to 3.6v and 100 relates to fully charged batt

#ifdef LOGBOOK_ENABLE
jump_t current_jump;
#endif

#ifdef SNAPSHOT_ENABLE
// Used in snapshot saver
uint16_t accumulated_altitude;
int accumulated_speed;
#endif

#if defined(LOGBOOK_ENABLE) || defined(SNAPSHOT_ENABLE)
uint16_t total_jumps;
#endif

char textbuf[48];
char bigbuf[SNAPSHOT_SIZE]; // also used in snapshot writer

uint8_t timeWhileBtnMenuPressed = 0;
uint8_t timeToTurnBacklightOn = 0;

// Prototypes
#ifdef GREETING_ENABLE
void ShowText(const uint8_t x, const uint8_t y, const char* text);
#endif
char myMenu(char *menudef, char event = ' ');
void showVersion();
void wake() {
#if defined (__AVR_ATmega32U4__)
    INTACK = true;
#endif
}
bool SetDate(timestamp_t &date);
#ifdef ALARM_ENABLE
bool checkAlarm();
#endif
void PowerOff(bool verbose = true);
void memoryDump();

void pciSetup(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

#if defined (__AVR_ATmega32U4__)
ISR (PCINT0_vect) { // the only PCINT for 32u4
    INTACK = true;
}
#endif

#if defined(__AVR_ATmega328P__)
ISR (PCINT2_vect) { // handle pin change interrupt for D0 to D5 here (328p)
    INTACK = true;
}

ISR (PCINT1_vect) { // handle pin change interrupt for A0 to A5 here (328p)
}

#endif

void loadJumpProfile() {
    EEPROM.get(EEPROM_JUMP_PROFILES + ((settings.jump_profile_number >> 2) * sizeof(jump_profile_t)), jump_profile);
#ifdef AUDIBLE_SIGNALS_ENABLE
    EEPROM.get(EEPROM_AUDIBLE_SIGNALS + ((settings.jump_profile_number) * sizeof(audible_signals_t)), audible_signals);
#endif    
}

void setup() {
    // Configure keyboard and enable pullup resistors
    pinMode(PIN_BTN1, INPUT_PULLUP);
    pinMode(PIN_BTN2, INPUT_PULLUP);
    pinMode(PIN_BTN3, INPUT_PULLUP);
    pinMode(PIN_INTERRUPT, INPUT_PULLUP);
    pinMode(PIN_LIGHT, OUTPUT);
    DISPLAY_LIGHT_OFF;

    // Turn ON hardware
    pinMode(PIN_HWPWR, OUTPUT);
    digitalWrite(PIN_HWPWR, 1);
    Wire.begin();
//    Wire.setClock(WIRE_SPEED);
    delay(50); // Wait hardware to start

    rtc.init();
    rtc.enableHeartbeat();  // reset alarm flags, start generate seed sequence
    rtc.readTime();
#ifdef ALARM_ENABLE
    rtc.readAlarm();
#endif

    // Read presets
    altitude_to_show = 0;
    powerMode = previousPowerMode = MODE_PREFILL;
    previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
    vspeedPtr = 0;
    interval_number = 0;
#if defined(LOGBOOK_ENABLE) || defined(SNAPSHOT_ENABLE)
    EEPROM.get(EEPROM_JUMP_COUNTER, total_jumps);
#endif

    EEPROM.get(EEPROM_SETTINGS, settings);
    loadJumpProfile();

    LED_show(0, 0, 0);

    initSound();

    u8g2.begin();
#if DISPLAY==DISPLAY_NOKIA1201
    u8g2.setContrast((uint8_t)(settings.contrast << 4) + 15);
#endif
    u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
    u8g2.setDisplayRotation((settings.display_rotation) ? U8G2_R0 : U8G2_R2);

#ifdef ALARM_ENABLE
    if (checkAlarm())
        PowerOff(false);
#endif

    //Configure the sensor
    myPressure.begin(); // Get sensor online

    heartbeat = ByteToHeartbeat(settings.auto_power_off);

    if (myPressure.readAltitude() < 450) { // "Reset in airplane" - Disable greeting message if current altitude more than 450m
        DISPLAY_LIGHT_ON;
#if defined(__AVR_ATmega328P__)
        Serial.begin(SERIAL_SPEED);
        delay(2000);
        if (Serial.available() && Serial.read() == '?') {
            memoryDump();
        }
#endif

#ifdef GREETING_ENABLE
        SET_MAX_VOL;
        sound(SIGNAL_WELCOME);
        
        // Show greeting message
        char tmpBuf[16];
#ifdef CUSTOM_GREETING_ENABLE
        uint8_t hasCustomMessage;
        EEPROM.get(0x1f, hasCustomMessage);
        if (hasCustomMessage == 0x41) {
            EEPROM.get(0x130, tmpBuf);
        }
        else
#endif
            strcpy_P(tmpBuf, MSG_HELLO);
        ShowText(16, 30, tmpBuf);

        DISPLAY_LIGHT_ON;
#endif // GREETING_ENABLE

#if defined(__AVR_ATmega328P__)
        if (Serial.available() && Serial.read() == '?') {
            memoryDump();
        }
#endif

#ifdef GREETING_ENABLE
        showVersion();
        delay(7000);
        DISPLAY_LIGHT_OFF;
#endif // GREETING_ENABLE

#if defined(__AVR_ATmega328P__)
        Serial.end();
        pinMode(0, INPUT);
        pinMode(1, INPUT);
#endif
    }
    // Wake by pin-change interrupt from RTC that generates 1Hz 50% duty cycle
    pciSetup(PIN_INTERRUPT_HEARTBEAT);
}

#ifdef ALARM_ENABLE
bool checkAlarm() {
    if (rtc.alarm_enable && rtc.hour == rtc.alarm_hour && rtc.minute == rtc.alarm_minute) {
        rtc.alarm_enable = false;
        rtc.setAlarm();

        sprintf_P(bigbuf, PSTR("%02d:%02d"), rtc.hour, rtc.minute);
        u8g2.setFont(font_alarm);
        u8g2.firstPage();
        do {
            u8g2.drawUTF8((DISPLAY_WIDTH - 80) >> 1, (DISPLAY_HEIGHT >> 1) + 12, bigbuf);
        } while(u8g2.nextPage());

        while (BTN2_PRESSED);
        for (uint8_t i = 0; i < 90; ++i) {
#ifdef SOUND_VOLUME_CONTROL_ENABLE
            byte vol = i >> 3;
            if (vol > 3)
                vol = 3;
            setVol(vol);
#endif
            sound(SIGNAL_2SHORT);
            DISPLAY_LIGHT_ON;
            delay(500);
            DISPLAY_LIGHT_OFF;
            delay(500);
            if (BTN2_PRESSED)
                break;
        }
        return true;
    }

    return false;
}
#endif

#ifdef SNAPSHOT_ENABLE
void saveToJumpSnapshot() {
    if (current_jump.total_jump_time < 2045)
        current_jump.total_jump_time++;

    if (current_jump.total_jump_time >= (SNAPSHOT_SIZE * 2) || (current_jump.total_jump_time & 1) /* write once per second */ )
        return;
    int speed_now = current_altitude - accumulated_altitude;
    // Limit acceleration (in fact, this is exactly what we store) with 1.1g down, 2,5g up.
    // Anyway, while freefall, acceleration more than 1g down is not possible.
    int8_t accel = (speed_now - accumulated_speed);
    if (accel < -11)
        accel = -11;
    if (accel > 25)
        accel = 25;
    bigbuf[current_jump.total_jump_time >> 1] = (char)(speed_now - accumulated_speed);
    accumulated_speed += accel;
    accumulated_altitude += accumulated_speed;
}
#endif

//
// Main state machine
// Ideas got from EZ430 project at skycentre.net
//
void processAltitude() {
    if (previous_altitude == CLEAR_PREVIOUS_ALTITUDE) { // unknown delay or first run; just save
        previous_altitude = current_altitude;
        return;
    }
    
    current_speed = (current_altitude - previous_altitude) << 1; // ok there; sign bit will be kept because <int> is big enough

    vspeed[vspeedPtr & 31] = (int8_t)current_speed;
    // Calculate averages
    uint8_t startPtr = ((uint8_t)(vspeedPtr)) & 31;
    average_speed_32 = 0;
    for (uint8_t i = 0; i < 32; i++) {
        average_speed_32 += vspeed[(startPtr - i) & 31];
        if (i == 7)
            average_speed_8 = average_speed_32 / 8; // sign extension used
    }
    average_speed_32 /= 32;
    vspeedPtr++;

    int jitter = current_altitude - altitude_to_show;
    bool jitter_in_range = (jitter < 5 && jitter > -5);

    // Compute altitude to show
    if (powerMode == MODE_ON_EARTH) {
        // Jitter compensation
        if (jitter_in_range) {
            // Inside jitter range. Try to correct zero drift.
            if (altitude_to_show == 0) {
                zero_drift_sense += jitter;

                if (zero_drift_sense < 8) {
                    // Do correct zero drift
                    myPressure.zero();
                    // Altitude already shown as 0. current_altitude is not needed anymore.
                    // It will be re-read in next cycle and assumed to be zero, as we corrected
                    // zero drift.
                    zero_drift_sense = 128;
                }
            } else {
                // Not at ground
                zero_drift_sense = 128;
            }
        } else {
            // jitter out of range
            altitude_to_show = current_altitude;
            zero_drift_sense = 128;
        }
    } else {
        // Do not perform zero drift compensation and jitter correction in modes other than MODE_ON_EARTH
        altitude_to_show = current_altitude;
        zero_drift_sense = 128;
    }

    if (powerMode == MODE_PREFILL) {
        // Change mode to ON_EARTH if vspeed has been populated (approx. 16s after enter to this mode)
        if (vspeedPtr > 32) {
            // set to zero if we close to it
            if (settings.zero_after_reset && current_altitude < 450) {
                altitude_to_show = 0;
                myPressure.zero();
            }
            powerMode = MODE_ON_EARTH;
        }
    } else

    //
    // ************************
    // ** MAIN STATE MACHINE **
    // ************************
    //
    // Due to compiler optimization, **DO NOT** use `case` here.
    // Also, **DO NOT** remove `else`'s.

    if (powerMode == MODE_ON_EARTH) {
        if ( /*(current_altitude > 250) ||*/ (current_altitude > 30 && average_speed_8 > 1) )
            powerMode = MODE_IN_AIRPLANE;
        else if (average_speed_8 <= FREEFALL_TH) {
            powerMode = MODE_FREEFALL;
#ifdef LOGBOOK_ENABLE
            current_jump.total_jump_time = 2047; // do not store this jump in logbook
#endif
        }
    } else
    
    if (powerMode == MODE_IN_AIRPLANE) {
        if (current_speed <= EXIT_TH) {
            powerMode = MODE_EXIT;
#ifdef LOGBOOK_ENABLE
            current_jump.exit_altitude = (current_altitude) >> 1;
            current_jump.exit_timestamp = rtc.getTimestamp();
            current_jump.total_jump_time = current_jump.max_freefall_speed_ms = 0;
#endif
#ifdef SNAPSHOT_ENABLE
            bigbuf[0] = (char)current_speed;
            accumulated_altitude = current_altitude;
            accumulated_speed = current_speed;
#endif
        }
        else if (average_speed_32 < 1 && current_altitude < 25)
            powerMode = MODE_ON_EARTH;
    } else
    
    if (powerMode == MODE_EXIT) {
#ifdef SNAPSHOT_ENABLE
        saveToJumpSnapshot();
#endif
        if (current_speed <= BEGIN_FREEFALL_TH) {
            powerMode = MODE_BEGIN_FREEFALL;
        }
        else if (current_speed > EXIT_TH)
            powerMode = MODE_IN_AIRPLANE;
    } else
    
    if (powerMode == MODE_BEGIN_FREEFALL) {
#ifdef SNAPSHOT_ENABLE
        saveToJumpSnapshot();
#endif
        if (current_speed <= FREEFALL_TH)
            powerMode = MODE_FREEFALL;
        else if (current_speed > BEGIN_FREEFALL_TH)
            powerMode = MODE_EXIT;
    } else
    
    if (powerMode == MODE_FREEFALL) {
#ifdef SNAPSHOT_ENABLE
        saveToJumpSnapshot();
#endif
        if (current_speed > PULLOUT_TH)
            powerMode = MODE_PULLOUT;
#ifdef LOGBOOK_ENABLE
        current_jump.deploy_altitude = current_altitude >> 1;
        current_jump.deploy_time = current_jump.total_jump_time;
        int8_t freefall_speed = 0 - average_speed_8;
        if (freefall_speed > current_jump.max_freefall_speed_ms)
            current_jump.max_freefall_speed_ms = freefall_speed;
#endif
    } else
    
    if (powerMode == MODE_PULLOUT) {
#ifdef SNAPSHOT_ENABLE
        saveToJumpSnapshot();
#endif
        if (current_speed <= PULLOUT_TH)
            powerMode = MODE_FREEFALL;
        else if (current_speed > OPENING_TH) {
            powerMode = MODE_OPENING;
        }
    } else
    if (powerMode == MODE_OPENING) {
#ifdef SNAPSHOT_ENABLE
        saveToJumpSnapshot();
#endif
        if (current_speed <= OPENING_TH)
            powerMode = MODE_PULLOUT;
        else if (average_speed_8 > UNDER_PARACHUTE_TH) {
            powerMode = MODE_UNDER_PARACHUTE;
#ifdef LOGBOOK_ENABLE
            current_jump.canopy_altitude = current_jump.deploy_altitude - (current_altitude >> 1);
#endif
        }
    } else
    if (powerMode == MODE_UNDER_PARACHUTE) {
#ifdef SNAPSHOT_ENABLE
        saveToJumpSnapshot();
#endif
        if (average_speed_32 > -1 && current_altitude < 25) {
            powerMode = MODE_ON_EARTH;
            // try to lock zero altitude
            altitude_to_show = 0;
#ifdef SNAPSHOT_ENABLE
            saveSnapshot();
#endif
#ifdef LOGBOOK_ENABLE
            saveJump();
#endif
#if defined(LOGBOOK_ENABLE) || defined(SNAPSHOT_ENABLE)
            total_jumps++;
            EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
#endif
        }
    }
    
}

uint8_t airplane_300m = 0;
void ShowLEDs() {
    if ((powerMode > MODE_ON_EARTH && powerMode < MODE_PREFILL && settings.backlight == 2)
        || (powerMode > MODE_IN_AIRPLANE && powerMode < MODE_PREFILL && settings.backlight == 2)
        || settings.backlight == 1
        || (timeToTurnBacklightOn > 0 && timeWhileBtnMenuPressed < 32))
        DISPLAY_LIGHT_ON;
    else
        DISPLAY_LIGHT_OFF;
    
    // Blink as we entering menu
    switch (timeWhileBtnMenuPressed) {
        case 0:
            break; // continue normal processing
        case 1:
        case 3:
        case 5:
            LED_show(0, 0, 180);
            return;
        case 7:
            LED_show(0, 80, 0);
            return;
        case 10:
        case 12:
            LED_show(255, 0, 0);
            return;
        default: // 2, 4, 6, 8, 9, 11, 13+ => off all LEDs
            LED_show(0, 0, 0);
            return;
    }
    
#ifdef AUDIBLE_SIGNALS_ENABLE
    if (powerMode != MODE_ON_EARTH && settings.use_audible_signals && current_altitude > 0 && previous_altitude > 0) {
        for (int i = 7; i >=0; i--) {
            if (current_altitude <= audible_signals.signals[i] && previous_altitude > audible_signals.signals[i]) {
                SET_VOL;
                sound(128 + i);
                break;
            }
        }
    }
#endif            

    switch (powerMode) {
        case MODE_IN_AIRPLANE:
            if (settings.use_led_signals) {
                if (current_altitude >= 300) {
                    if (airplane_300m <= 6 && current_altitude < 900) {
                        if (++airplane_300m & 1) {
                            LED_show(0, 140, 0); // green blinks 3 times at 300m but not above 900m
#ifdef AUDIBLE_SIGNALS_ENABLE
                            if (settings.use_audible_signals)
                                SET_VOL;
                                sound(SIGNAL_1MEDIUM);
#endif
                        } else {
                            LED_show(0, 0, 0); // green blinks 3 times at 300m but not above 900m
                        }
                    } else {
                        if (current_altitude > 900) {
                            LED_show(0, (interval_number & 15) ? 0 : 80, 0); // green blinks once per 8s above 900m
                        } else {
                            LED_show((interval_number & 15) ? 0 : 255, (interval_number & 15) ? 0 : 80, 0); // yellow blinks once per 8s between 300m and 900m
                        }
                    }
                } else {
                    LED_show((interval_number & 15) ? 0 : 120, 0, 0); // red blinks once per 8s below 300m
                }
            }
            return;
        case MODE_EXIT:
        case MODE_BEGIN_FREEFALL:
        case MODE_FREEFALL:
        case MODE_PULLOUT:
        case MODE_OPENING:
            if (settings.use_led_signals) {
                if (current_altitude < 1000) {
                    LED_show(255, 0, 0); // red ligth below 1000m
                } else if (current_altitude < 1200) {
                    LED_show(255, 80, 0); // yellow ligth between 1200 and 1000m
                } else if (current_altitude < 1550) {
                    LED_show(0, 255, 0); // green ligth between 1550 and 1200m
                } else {
                    LED_show(0, 0, 255); // blue ligth above 1550m
                }
            }
            return;

        case MODE_PREFILL:
            LED_show((interval_number & 3) ? 0 : 120, 0, 0); // red blinks once per 2s until prefill finished
            return;

        case MODE_ON_EARTH:
            if (interval_number < 8) {
                if (interval_number & 1) {
                    LED_show(0, 0, 0); // green blinks to indicate that altimeter is ready
                    noSound();
                } else {
                    LED_show(0, 80, 0); // green blinks to indicate that altimeter is ready
                    SET_VOL;
                    sound(SIGNAL_1MEDIUM);
                }
            } else
                break; // turn LED off
            return;
    }

    airplane_300m = 0;
    LED_show(0, 0, 0);
//    noSound();
}

#ifdef GREETING_ENABLE
void ShowText(const uint8_t x, const uint8_t y, const char* text) {
    u8g2.setFont(font_hello);
    DISPLAY_LIGHT_ON;
    uint8_t maxlen = strlen(text);
    for (uint8_t i = 1; i <= maxlen; i++) {
        strcpy(bigbuf, text);
        bigbuf[i] = 0;
        u8g2.firstPage();
        do {
            u8g2.drawUTF8(x, y, bigbuf);
        } while(u8g2.nextPage());
        delay(250);
    }
    delay(5000);
    DISPLAY_LIGHT_OFF;
}
#endif

void PowerOff(bool verbose = true) {
#ifdef GREETING_ENABLE
    if (verbose) {
        // Show bye message
        char tmpBuf[16];
#ifdef CUSTOM_GREETING_ENABLE
        uint8_t hasCustomMessage;
        EEPROM.get(0x1f, hasCustomMessage);
        if (hasCustomMessage == 0x41) {
            EEPROM.get(0x140, tmpBuf);
        }
        else
#endif
            strcpy_P(tmpBuf, MSG_BYE);
        ShowText(16, 30, tmpBuf);
    }
#endif
    noSound();
    rtc.disableHeartbeat(); // Do it before enabling alarm, because it will tirn alarm off on pcf8583
#ifdef ALARM_ENABLE
    rtc.enableAlarmInterrupt();
#endif


    // turn off i2c
    pinMode(SCL, INPUT);
    pinMode(SDA, INPUT);
    // turn off SPI
    SPI.end();
    pinMode(MOSI, INPUT); // todo: set all portB as input
    pinMode(MISO, INPUT);
    pinMode(SCK, INPUT);
    pinMode(SS, INPUT);
#ifdef DC_PIN    
    pinMode(DC_PIN, INPUT);
#endif

    // After we turned off all peripherial connections, turn peripherial power OFF too.
    digitalWrite(PIN_HWPWR, 0);

#if defined(__AVR_ATmega328P__)
    // Wake by pin-change interrupt on any key
    pciSetup(PIN_BTN1);
    pciSetup(PIN_BTN2);
    pciSetup(PIN_BTN3);
    Serial.end();
    pinMode(0, INPUT);
    pinMode(1, INPUT);
#endif
    
    do {
#if defined(__AVR_ATmega32U4__)
        // Wake by BTN2 (Middle button)
        attachInterrupt(digitalPinToInterrupt(PIN_BTN2), wake, LOW);
#endif
        // Also wake by alarm. TODO.
#ifdef ALARM_ENABLE
        attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
#endif
        off_forever;
#ifdef ALARM_ENABLE
        detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
#endif
#if defined(__AVR_ATmega32U4__)
        detachInterrupt(digitalPinToInterrupt(PIN_BTN2));
#endif
    } while (!checkWakeCondition());
    resetFunc();
}

#if defined(__AVR_ATmega328P__)
uint8_t checkWakeCondition () {
    uint8_t pin;
    if (BTN1_PRESSED)
        pin = PIN_BTN1;
    else if (BTN2_PRESSED)
        pin = PIN_BTN2;
    else if (BTN3_PRESSED)
        pin = PIN_BTN3;
    else
#ifdef ALARM_ENABLE
        return !digitalRead(PIN_INTERRUPT); // alarm => interrupt => wake
#else
        return 0;
#endif
    LED_show(0, 0, 80, 400);
    for (uint8_t i = 1; i < 193; ++i) {
        if (digitalRead(pin))
            return 0;
        if (! (i & 63))
            LED_show(0, 0, 80, 200);
        delay(15);
    }
    return 1;
}
#endif

#if defined(__AVR_ATmega32U4__)
uint8_t checkWakeCondition () {
    // Determine wake condition
    if (BTN2_PRESSED) {
        // Wake by awake button. Should be kept pressed for 3s
        LED_show(0, 0, 80, 400);
        for (uint8_t i = 1; i < 193; ++i) {
        if (BTN2_RELEASED)
            return 0;
        if (! (i & 63))
            LED_show(0, 0, 80, 200);
        delay(15);
        }
        return 1;
    }
#ifdef ALARM_ENABLE
    return !digitalRead(PIN_INTERRUPT); // alarm => interrupt => wake
#else
    return 0;
#endif
}
#endif

void showVersion() {
    u8g2.setFont(font_menu);
    sprintf_P(bigbuf, MSG_VERSION,
        PLATFORM_1, PLATFORM_2, PLATFORM_3, PLATFORM_4,
        SERIAL_SPEED);
    myMenu(bigbuf, 'z');
}

// Returns ' ' on timeout; otherwise, current position
// event = '*' => show menu and wait for keypress, no navigation performed.
// event == 'z' => show menu and exit immediately.
char myMenu(char *menudef, char event = ' ') {
    uint16_t ptr = 0;
    char ptrs[20];
    uint8_t menupos = 0;
    uint8_t number_of_menu_lines = 255;
    while(menudef[ptr]) {
        if (menudef[ptr] == '\n') {
            menudef[ptr] = '\0';
            number_of_menu_lines++;
            ptrs[number_of_menu_lines] = menudef[ptr + 1];
            if (ptrs[number_of_menu_lines] == event)
                menupos = number_of_menu_lines + 1;
        }
        ptr++;
    }
    
    uint8_t firstline = 1;

    for (;;) {
        // check if we need to update scroll
        if (menupos != 0 && menupos != 255) {
            if ((menupos - firstline) >= DISPLAY_LINES_IN_MENU)
                firstline = menupos - (DISPLAY_LINES_IN_MENU - 1);
            if (menupos < firstline)
                firstline = menupos;
        }
        u8g2.firstPage();
        do {
            ptr = 0;
            uint8_t line = 1;
            // Print title and find ptr
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT, menudef);
            while (menudef[ptr++]); // set to the beginning of the next line
            u8g2.drawHLine(0, MENU_FONT_HEIGHT + 1, DISPLAY_WIDTH-1);
            while (menudef[ptr] != 0) {
                if ((line >= firstline) && (line - firstline) < DISPLAY_LINES_IN_MENU) {
                    if (menupos != 0 && menupos != 255)
                        menudef[ptr] = (menupos == line) ? '}' : ' ';
                    u8g2.drawUTF8(0, (MENU_FONT_HEIGHT * (line - firstline)) + (MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 2), (char*)(menudef + ptr));
                }
                line++;
                while (menudef[ptr++]); // set to the beginning of the next line
            }
        } while (u8g2.nextPage());
        if (event == 'z')
            return 'z';
        uint8_t key = getKeypress();
        if (event == '*')
            return (key == 255) ? 'z' : (char)key;
        if (key == PIN_BTN2)
            return ptrs[menupos - 1];
        switch (key) {
            case PIN_BTN1:
                // up
                menupos--;
                if (menupos == 0)
                    menupos = number_of_menu_lines;
                break;
            case PIN_BTN3:
                // down
                menupos++;
                if (menupos > number_of_menu_lines)
                    menupos = 1;
                break;
        }
    }
}

const char *profileChars = "SFCW";
char getProfileChar(uint8_t profileid) {
    return profileChars[profileid & 3];
}

void current_jump_to_bigbuf(uint16_t jump_to_show) {
    uint16_t freefall_time = current_jump.deploy_time >> 1;
    int average_freefall_speed_ms = ((current_jump.exit_altitude - current_jump.deploy_altitude) << 1) / freefall_time;
    int average_freefall_speed_kmh = (int)(3.6f * average_freefall_speed_ms);
    int max_freefall_speed_kmh = (int)(3.6f * current_jump.max_freefall_speed_ms);
    uint8_t day = current_jump.exit_timestamp.day;
    uint8_t month = current_jump.exit_timestamp.month;
    uint16_t year = current_jump.exit_timestamp.year;
    uint8_t hour = current_jump.exit_timestamp.hour;
    uint8_t minute = current_jump.exit_timestamp.minute;
    int16_t exit_altitude = current_jump.exit_altitude << 1;
    int16_t deploy_altitude = current_jump.deploy_altitude << 1;
    int16_t canopy_altitude = (current_jump.deploy_altitude - current_jump.canopy_altitude) << 1;
    uint16_t max_freefall_speed_ms = current_jump.max_freefall_speed_ms;

    sprintf_P(bigbuf, MSG_JUMP_CONTENT,
        jump_to_show, total_jumps, getProfileChar(current_jump.profile),
        day, month, year, hour, minute,
        exit_altitude, deploy_altitude,
        canopy_altitude, freefall_time,
        average_freefall_speed_ms, average_freefall_speed_kmh,
        max_freefall_speed_ms, max_freefall_speed_kmh
        );
}

#ifdef AUDIBLE_SIGNALS_ENABLE
char setAudibleSignals() {
    char event;
    uint8_t pos = 0;

    for (;;) {
        if (pos == 255)
            pos = 9;
        if (pos == 10)
            pos = 0;
        sprintf_P(bigbuf, PSTR("%c%c\n1%c%4u 5%c%4u\n2%c%4u 6%c%4u\n3%c%4u 7%c%4u\n4%c%4u 8%c%4u\n%cОтмена %cOK\n"),
            getProfileChar(settings.jump_profile_number >> 2), '1' + (settings.jump_profile_number & 3),
            pos == 0 ? '}' : ':', audible_signals.signals[0],
            pos == 4 ? '}' : ':', audible_signals.signals[4],
            pos == 1 ? '}' : ':', audible_signals.signals[1],
            pos == 5 ? '}' : ':', audible_signals.signals[5],
            pos == 2 ? '}' : ':', audible_signals.signals[2],
            pos == 6 ? '}' : ':', audible_signals.signals[6],
            pos == 3 ? '}' : ':', audible_signals.signals[3],
            pos == 7 ? '}' : ':', audible_signals.signals[7],
            pos == 8 ? '}' : ' ',
            pos == 9 ? '}' : ' '
        );
        event = myMenu(bigbuf, '*');
        if (event == 'z')
            return 'z';
        if (event == PIN_BTN1) {
            if (pos > 7)
                pos--;
            else if (pos < 4) {
                if (audible_signals.signals[pos] >= 10)
                    audible_signals.signals[pos] -= 10;
            } else {
                if (audible_signals.signals[pos] >= 5)
                    audible_signals.signals[pos] -= 5;
            }
        }
        else if (event == PIN_BTN3) {
            if (pos > 7)
                pos++;
            else if (pos < 4) {
                if (audible_signals.signals[pos] <= 4990)
                    audible_signals.signals[pos] += 10;
                else
                    audible_signals.signals[pos] += 5000;
            } else {
                if (audible_signals.signals[pos] <= 4995)
                    audible_signals.signals[pos] += 5;
                else
                    audible_signals.signals[pos] += 5000;
            }
        }
        else if (event == PIN_BTN2) {
            if (pos < 8)
                pos++;
            else return (pos == 8) ? ' ' : PIN_BTN2;
        }
    };
}
#endif

void userMenu() {
    DISPLAY_LIGHT_ON; // it will be turned off, if needed, in ShowLEDs(..)

    u8g2.setFont(font_menu);

    char event = ' ';
    do {
        char bl_char = '~';
        if (settings.backlight == 0)
            bl_char = '-';
        if (settings.backlight == 2)
            bl_char = '*';
        if (settings.backlight == 3)
            bl_char = '+';
        sprintf_P(bigbuf, PSTR(
            MSG_MENU
            MSG_EXIT
            MSG_SET_TO_ZERO
            MSG_PROFILE
            MSG_BACKLIGHT
#ifdef LOGBOOK_ENABLE
            MSG_LOGBOOK
#endif
            MSG_SETTINGS
            MSG_POWEROFF),
            getProfileChar(settings.jump_profile_number >> 2),
#ifdef AUDIBLE_SIGNALS_ENABLE
            '1' + (settings.jump_profile_number & 3),
#else
            ' ',
#endif
            bl_char);
        event = myMenu(bigbuf, event);
        switch (event) {
            case 'P':
#ifdef AUDIBLE_SIGNALS_ENABLE
                settings.jump_profile_number++;
#else
                settings.jump_profile_number++;
#endif
                loadJumpProfile();
                break;
            case '0': {
                // Set to zero
                if (powerMode == MODE_ON_EARTH || powerMode == MODE_PREFILL || powerMode == MODE_DUMB) {
                    // Zero reset allowed on earth, in prefill and in dumb mode only.
                    myPressure.zero();
                    current_altitude = altitude_to_show = 0;
                    LED_show(0, 80, 0, 250);
                    return;
                } else {
                    LED_show(255, 0, 0, 250);
                    break;;
                }
            }
            case 'B':
                // Backlight turn on/off
                settings.backlight++;
                break;
#ifdef LOGBOOK_ENABLE
            case 'L': {
                // Logbook
                char logbook_event = ' ';
                do {
                    strcpy_P(bigbuf, PSTR(
                        MSG_LOGBOOK_TITLE
                        MSG_EXIT
                        MSG_LOGBOOK_VIEW
#ifdef TEST_JUMP_ENABLE
                        MSG_LOGBOOK_REPLAY_JUMP
#endif
                        MSG_LOGBOOK_CLEAR));
                    logbook_event = myMenu(bigbuf, logbook_event);
                    switch (logbook_event) {
                        case  ' ':
                            // Exit submenu
                            break;
                        case 'V': {
                            // view logbook
                            if (total_jumps == 0)
                                break; // no jumps, nothing to show
                            uint16_t jump_to_show = total_jumps;
                            uint8_t logbook_view_event;
                            for (;;) {
                                if (jump_to_show == 0 || (jump_to_show + LOGBOOK_SIZE) == total_jumps)
                                    jump_to_show = total_jumps;
                                if (jump_to_show > total_jumps)
                                    jump_to_show = (total_jumps > LOGBOOK_SIZE) ? total_jumps - LOGBOOK_SIZE : 1;

                                // Read jump from logbook
                                loadJump(jump_to_show - 1);
                                current_jump_to_bigbuf(jump_to_show);
                                logbook_view_event = (uint8_t)myMenu(bigbuf, '*');
                                if (logbook_view_event == PIN_BTN1)
                                    jump_to_show--;
                                if (logbook_view_event == PIN_BTN2)
                                    break;
                                if (logbook_view_event == PIN_BTN3)
                                    jump_to_show++;
                                if (logbook_view_event == 'z')
                                    return;;
                            };
                            break;
                        }
#ifdef TEST_JUMP_ENABLE
                        case 'R':
                            // replay latest jump
                            myPressure.debugPrint();
                            return;
//                            break; // not implemented
#endif
                            
                        case 'C': {
                            // Erase logbook
                            strcpy_P(bigbuf, PSTR(
                                MSG_LOGBOOK_CLEAR_CONFIRM_TITLE
                                MSG_CONFIRM_NO
                                MSG_CONFIRM_YES));
                            char confirmCrearLogbook = myMenu(bigbuf, ' ');
                            if (confirmCrearLogbook == 'z')
                                return;
                            if (confirmCrearLogbook == 'Y') {
                                // Really clear logbook
                                total_jumps = 0;
                                EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
                                return; // exit logbook menu
                            }
                            break;
                        }
                        default:
                            // timeout
                            return;
                    } // switch (logbook_event)
                } while (logbook_event != ' ');
                // Logbook
                break;
            }
#endif            
            case 'S': {
                // "Settings" submenu
                char eventSettings = ' ';
                do {
                    char power_mode_char = powerMode == MODE_DUMB ? '-' : '~';
                    char zero_after_reset = settings.zero_after_reset ? '~' : '-';
                    textbuf[0] = '\0';
#ifdef ALARM_ENABLE
                    if (rtc.alarm_enable & 1)
                        sprintf_P(textbuf, PSTR("%02d:%02d"), rtc.alarm_hour, rtc.alarm_minute);
#endif
                    sprintf_P(bigbuf, PSTR(
                        MSG_SETTINGS_TITLE
                        MSG_EXIT
                        MSG_SETTINGS_SET_TIME
                        MSG_SETTINGS_SET_DATE
#ifdef ALARM_ENABLE
                        MSG_SETTINGS_SET_ALARM
#endif
#ifdef AUDIBLE_SIGNALS_ENABLE
                        MSG_SETTINGS_SET_SIGNALS
                        MSG_SETTINGS_SET_SIGNALS_TEST
#ifdef SOUND_VOLUME_CONTROL_ENABLE
                        MSG_SETTINGS_LOUDNESS
#endif
                        MSG_SETTINGS_SET_SIGNALS_BUZZER
#endif
                        MSG_SETTINGS_SET_SIGNALS_LED
                        MSG_SETTINGS_SET_MODE
                        MSG_SETTINGS_SET_AUTO_ZERO
                        MSG_SETTINGS_SET_AUTO_POWER_OFF
                        MSG_SETTINGS_SET_SCREEN_ROTATION
#if DISPLAY==DISPLAY_NOKIA1201
                        MSG_SETTINGS_SET_SCREEN_CONTRAST
#endif
                        MSG_SETTINGS_ABOUT
#if defined (__AVR_ATmega32U4__)
                        MSG_SETTINGS_PC_LINK
#endif
                        ),
#ifdef ALARM_ENABLE
                        textbuf,
#endif
#ifdef AUDIBLE_SIGNALS_ENABLE
#ifdef SOUND_VOLUME_CONTROL_ENABLE
                        settings.volume + 1,
#endif
                        settings.use_audible_signals ? '~' : '-',
#endif
                        settings.use_led_signals ? '~' : '-',
                        power_mode_char,
                        zero_after_reset,
                        HeartbeatValue(settings.auto_power_off),
#if DISPLAY==DISPLAY_NOKIA1201
                        settings.contrast,
#endif
                        0 /* Dummy value */);
                    eventSettings = myMenu(bigbuf, eventSettings);
                    switch (eventSettings) {
                        case ' ':
                            break; // Exit menu
#ifdef AUDIBLE_SIGNALS_ENABLE
                        case 'S': {
                            char signalEvent = setAudibleSignals();
                            if (signalEvent == PIN_BTN2)
                                EEPROM.put(EEPROM_AUDIBLE_SIGNALS + ((settings.jump_profile_number) * sizeof(audible_signals_t)), audible_signals);
                            loadJumpProfile();
                            if (signalEvent == 'z')
                                return; // timeout
                            break;
                        }
                        case 'B':
                            settings.use_audible_signals++;
                            break;
                        case 't': {
                            u8g2.setFont(font_altitude);
                            for (uint8_t i = 128; i <= 135; i++) {
                                u8g2.firstPage();
                                do {
                                    sprintf_P(textbuf, PSTR("%d"), i - 127);
                                    u8g2.drawUTF8(DISPLAY_WIDTH >> 1, DISPLAY_HEIGHT - 10, textbuf);
                                    
                                } while ( u8g2.nextPage() );
                                SET_VOL;
                                sound(i);
                                delay(5000);
                                if (BTN2_PRESSED)
                                    break;
                            }
                            u8g2.setFont(font_menu);
                            break;
                        }
#endif
                        case 'L':
                            settings.use_led_signals++;
                            break;
#ifdef SOUND_VOLUME_CONTROL_ENABLE
                        case 'l':
                            settings.volume++;
                            SET_VOL;
                            sound(SIGNAL_2SHORT);
                            break;
#endif
                        case 'T': {
                            // Time
                            rtc.readTime();
                            uint8_t hour = rtc.hour;
                            uint8_t minute = rtc.minute;
                            if (SetTime(hour, minute, PSTR(MSG_SETTINGS_SET_TIME_TITLE))) {
                                rtc.hour = hour;
                                rtc.minute = minute;
                                rtc.setTime();
                            }
                            break;
                        }
                        case 'D': {
                            // Date
                            rtc.readTime();
                            uint8_t day = rtc.day;
                            uint8_t month = rtc.month;
                            uint16_t year = rtc.year;
                            if (SetDate(day, month, year)) {
                                rtc.day = day;
                                rtc.month = month;
                                rtc.year = year;
                                rtc.setDate();
                            }
                            break;
                        }
#ifdef ALARM_ENABLE
                        case 'A': {
                            // Alarm
                            rtc.readAlarm();
                            char alarmEvent = ' ';
                            do {
                                sprintf_P(bigbuf, PSTR(
                                    MSG_SETTINGS_SET_ALARM_TITLE
                                    MSG_EXIT
                                    MSG_SETTINGS_SET_ALARM_TOGGLE
                                    MSG_SETTINGS_SET_ALARM_TIME),
                                    (rtc.alarm_enable & 1) ? '~' : '-', rtc.alarm_hour, rtc.alarm_minute);
                                alarmEvent = myMenu(bigbuf, alarmEvent);
                                switch(alarmEvent) {
                                    case 'S':
                                        rtc.alarm_enable = (++rtc.alarm_enable) & 1;
                                        break;
                                    case 'T': {
                                        uint8_t hour = rtc.alarm_hour;
                                        uint8_t minute = rtc.alarm_minute;
                                        if (SetTime(hour, minute, PSTR(MSG_SETTINGS_SET_ALARM_TITLE))) {
                                            rtc.alarm_hour = hour;
                                            rtc.alarm_minute = minute;
                                            rtc.alarm_enable = 1;
                                        }
                                    }
                                }
                            } while (alarmEvent != ' ');
                            rtc.setAlarm();
                            }
                            break;
#endif
                        case 'Q':
                            // Auto power mode
                            powerMode = powerMode == MODE_DUMB ? MODE_ON_EARTH : MODE_DUMB;
                            break;
                        case 'O': {
                            // Auto poweroff
                            settings.auto_power_off++;
                            heartbeat = ByteToHeartbeat(settings.auto_power_off);
                            break;
                        }
                        case 'R':
                            // Screen rotate
                            settings.display_rotation++;
                            u8g2.setDisplayRotation((settings.display_rotation) ? U8G2_R0 : U8G2_R2);
                            break;
#if DISPLAY==DISPLAY_NOKIA1201
                        case 'C':
                            // Contrast
                            settings.contrast++;
                            u8g2.setContrast((uint8_t)(settings.contrast << 4) + 15);
                            break;
#endif
                        case '0':
                            // Zero after reset
                            settings.zero_after_reset++;
                            break;
                        case 'V':
                            // About
                            showVersion();
                            getKeypress();
                            break;
#if defined (__AVR_ATmega32U4__)
                        case 'U': {
                            Serial.begin(SERIAL_SPEED); // Console
                            while (!Serial);
                            memoryDump();
                            Serial.end();
                            break;
                        }
#endif
                        default:
                            // timeout
                            return;
                    }
                    EEPROM.put(EEPROM_SETTINGS, settings);

                } while (eventSettings != ' ');
                break;
            }
            case 'X':
                // Power off
                PowerOff();
                break;
            default:
                return;
        }
        EEPROM.put(EEPROM_SETTINGS, settings);
    } while (event != ' ' && event != 'z');
}

bool SetDate(uint8_t &day, uint8_t &month, uint16_t &year) {
    uint8_t pos = 0;
    for(;;) {
        pos = (pos + 5) % 5;
        if (year > 2100)
            year = 2100;
        if (year < EPOCH)
            year = EPOCH;
        if (month > 12)
            month = 12;
        if (month == 0)
            month = 1;
        if (day == 0)
            day = 1;
        if (day > 30 && (month == 4 || month == 6 || month == 9 || month == 11))
            day = 30;
        if (month == 2 && (year & 3) == 0 && day > 29)
            day = 29;
        if (month == 2 && day > 28 && (year & 3) != 0)
            day = 28;
        u8g2.firstPage();
        do {
            sprintf_P(textbuf, PSTR(MSG_SETDATE_TITLE));
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT, textbuf);
            u8g2.drawHLine(0, MENU_FONT_HEIGHT + 1, DISPLAY_WIDTH-1);

            sprintf_P(textbuf, PSTR("%c%02d %c%02d %c%04d"),
                pos == 0 ? '}' : ' ',
                day,
                pos == 1 ? '}' : ' ',
                month,
                pos == 2 ? '}' : ' ',
                year
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 3, textbuf);

            sprintf_P(textbuf, PSTR(MSG_SETDATETIME_CANCEL),
                pos == 3 ? '}' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 5, textbuf);
            
            sprintf_P(textbuf, PSTR(MSG_SETDATETIME_OK),
                pos == 4 ? '}' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 7, textbuf);
            
        } while(u8g2.nextPage());
        uint8_t keyEvent = getKeypress();
        switch (keyEvent) {
            case PIN_BTN1: {
                switch (pos) {
                    case 0:
                        day--;
                        break;
                    case 1:
                        month--;
                        break;
                    case 2:
                        year--;
                        break;
                    case 3:
                    case 4:
                        pos--;
                        break;
                }
            }
            break;

            case PIN_BTN2: {
                switch (pos) {
                    case 0:
                    case 1:
                    case 2:
                        pos++;
                        break;
                    case 3:
                        return false;
                    case 4:
                        return true;
                }
            }
            break;

            case PIN_BTN3: {
                switch (pos) {
                    case 0:
                        day++;
                        break;
                    case 1:
                        month++;
                        break;
                    case 2:
                        year++;
                        break;
                    case 3:
                    case 4:
                        pos++;
                        break;
                }
            }
            break;

            default:
                // Timeout
                return false;
        }
    };
}

bool SetTime(uint8_t &hour, uint8_t &minute, char* title) {
    uint8_t pos = 0;
    for(;;) {
        pos &= 3;
        u8g2.firstPage();
        do {
            strcpy_P(textbuf, title);
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT, textbuf);
            u8g2.drawHLine(0, MENU_FONT_HEIGHT + 1, DISPLAY_WIDTH-1);

            sprintf_P(textbuf, PSTR("%c%02d:%02d%c"),
                pos == 0 ? '}' : ' ',
                hour,
                minute,
                pos == 1 ? '{' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 3, textbuf);

            sprintf_P(textbuf, PSTR(MSG_SETDATETIME_CANCEL),
                pos == 2 ? '}' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 5, textbuf);
            
            sprintf_P(textbuf, PSTR(MSG_SETDATETIME_OK),
                pos == 3 ? '}' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 7, textbuf);
            
        } while(u8g2.nextPage());
        uint8_t keyEvent = getKeypress();
        switch (keyEvent) {
            case PIN_BTN1: {
                switch (pos) {
                    case 0:
                        hour--;
                        if (hour > 23)
                            hour = 23;
                        break;
                    case 1:
                        minute--;
                        if (minute > 59)
                            minute = 59;
                        break;
                    case 2:
                    case 3:
                        pos--;
                        break;
                }
            }
            break;

            case PIN_BTN2: {
                switch (pos) {
                    case 0:
                    case 1:
                        pos++;
                        break;
                    case 2:
                        return false;
                    case 3:
                        return true;
                }
            }
            break;

            case PIN_BTN3: {
                switch (pos) {
                    case 0:
                        hour++;
                        if (hour > 23)
                            hour = 0;
                        break;
                    case 1:
                        minute++;
                        if (minute > 59)
                            minute = 0;
                        break;
                    case 2:
                    case 3:
                        pos++;
                        break;
                }
            }
            break;

            default:
                // Timeout
                return false;
        }
    };
}

void memoryDump() {
    sprintf_P(bigbuf, PSTR("\nPEGASUS_BEGIN\nPLATFORM %c%c%c%c\nVERSION " VERSION "\nLANG " LANGUAGE), PLATFORM_1, PLATFORM_2, PLATFORM_3, PLATFORM_4);
    Serial.print(bigbuf);
    // Dump settings
    sprintf_P(bigbuf, PSTR("\nLOGBOOK %c %04x %u\nSNAPSHOT %c %04x %u %u\nROM_BEGIN"),

#if defined(LOGBOOK_ENABLE)
#if LOGBOOK_LOCATION == LOCATION_FLASH
    'F', LOGBOOK_START, LOGBOOK_SIZE,
#else
    'E', LOGBOOK_START, LOGBOOK_SIZE,
#endif
#else
    'N', 0, 0,
#endif

#if defined(SNAPSHOT_ENABLE)
#if SNAPSHOT_JOURNAL_LOCATION == LOCATION_FLASH
    'F', SNAPSHOT_JOURNAL_START, SNAPSHOT_JOURNAL_SIZE, SNAPSHOT_SIZE
#else
    'E', SNAPSHOT_JOURNAL_START, SNAPSHOT_JOURNAL_SIZE, SNAPSHOT_SIZE
#endif
#else
    'N', 0, 0, 0
#endif
    );
    Serial.print(bigbuf);

    uint16_t i;
    for (i = 0; i < 1024; i++) {
        if (! (i & 15)) {
            sprintf_P(textbuf, PSTR("\n%04x:"), i);
            Serial.print(textbuf);
        }
        sprintf_P(textbuf, PSTR(" %02x"), EEPROM.read(i));
        Serial.print(textbuf);
    }
    Serial.print(F("\nROM_END"));

#if defined(FLASH_PRESENT)
    sprintf_P(bigbuf, PSTR("\nFLASH_BEGIN %u %u"), FLASH__PAGE_SIZE, FLASH__PAGES);
    Serial.print(bigbuf);
    for (uint16_t page = 0; page < FLASH__PAGES; page++) {
        LED_show((page & 1) ? 255 : 0, 0, 0);
        flashRom.readBytes(page * FLASH__PAGE_SIZE, FLASH__PAGE_SIZE, (uint8_t*)bigbuf);
        for (i = 0; i < FLASH__PAGE_SIZE; i++) {
            if (! (i & 15)) {
                sprintf_P(textbuf, PSTR("\n%04x:"), i + (page *FLASH__PAGE_SIZE));
                Serial.print(textbuf);
            }
            sprintf_P(textbuf, PSTR(" %02x"), (uint8_t)(bigbuf[i]));
            Serial.print(textbuf);
        }
    }
    LED_show(0, 0, 0);
    Serial.print(F("\nFLASH_END"));
#endif
    Serial.print(F("\nPEGASUS_END"));
}

void loop() {
    // Read sensors
    rtc.readTime();
    current_altitude = myPressure.readAltitude();
    previousPowerMode = powerMode;
    bool refresh_display = (powerMode != MODE_ON_EARTH);

    // TODO. Opening menu when jump snapshot has not been saved leads to a broken snapshot.
    // Either disable menu in automatic modes or erase snapshot in such cases.
    // On 32u4 and Mega it is possible to separate buffers while on 328p it is not.
    if (BTN2_PRESSED) {
        timeToTurnBacklightOn = 16;
        // Try to enter menu
        if (timeWhileBtnMenuPressed < 32)
            timeWhileBtnMenuPressed++;
    } else {
        if (timeToTurnBacklightOn > 0)
            timeToTurnBacklightOn--;
        if (timeWhileBtnMenuPressed > 6 && timeWhileBtnMenuPressed < 10) {
            if (powerMode > MODE_IN_AIRPLANE && powerMode < MODE_PREFILL) {
                // Forcibly Save snapshot for debug purposes
#ifdef SNAPSHOT_ENABLE
                saveSnapshot();
#endif
#ifdef LOGBOOK_ENABLE
                saveJump();
#endif
#if defined(LOGBOOK_ENABLE) || defined(SNAPSHOT_ENABLE)
                total_jumps++;
                EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
#endif
                powerMode = MODE_ON_EARTH;
            }
            LED_show(0, 0, 0);
            userMenu();
            refresh_display = true; // force display refresh
            previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
        }
        timeWhileBtnMenuPressed = 0;
    }
    
    if ((interval_number & 127) == 0 || powerMode == MODE_PREFILL) {
        // Check and refresh battery meter
        analogRead(PIN_BAT_SENSE);
        delay(10);
        batt = analogRead(PIN_BAT_SENSE);
        rel_voltage = (int8_t)((batt - settings.battGranulationD) * settings.battGranulationF);
        if (rel_voltage < 0)
            rel_voltage = 0;
        if (rel_voltage > 100)
            rel_voltage = 100;
#ifdef ALARM_ENABLE
        if (powerMode == MODE_ON_EARTH || powerMode == MODE_PREFILL || powerMode == MODE_DUMB)
            checkAlarm();
#endif
    }
    
    processAltitude();
    if (powerMode != previousPowerMode)
        interval_number = 0;

    refresh_display = refresh_display || !(interval_number & 31);
    
    ShowLEDs();

    previous_altitude = current_altitude;

    // Refresh display once per 8s in ON_EARTH, each heartbeat pulse otherwise
    if (refresh_display) {
        u8g2.firstPage();
        do {
            // Do not use bugbuf here, as it used for jump snapshot generation
            u8g2.setFont(font_status_line);

            sprintf_P(textbuf, PSTR("%02d.%02d.%04d %02d:%02d"), rtc.day, rtc.month, rtc.year, rtc.hour, rtc.minute);
            u8g2.drawUTF8(0, 6, textbuf);
            
            sprintf_P(textbuf, PSTR("%3d%%"), rel_voltage);
            u8g2.drawUTF8(DISPLAY_WIDTH - 16, 6, textbuf);
    
            u8g2.drawHLine(0, 7, DISPLAY_WIDTH-1);
            u8g2.drawHLine(0, DISPLAY_HEIGHT-8, DISPLAY_WIDTH-1);
    
            if (powerMode == MODE_ON_EARTH) {
#ifdef ALARM_ENABLE
                if (rtc.alarm_enable & 1)
                    sprintf_P(textbuf, PSTR("&%1d' %02d:%02d@"), powerMode, rtc.alarm_hour, rtc.alarm_minute);
                else
                    sprintf_P(textbuf, PSTR("&%1d' %d$"), powerMode, batt);
#else            
                sprintf_P(textbuf, PSTR("&%1d' %d$"), powerMode, batt);
#endif
            } else
                sprintf_P(textbuf, PSTR("&%1d' % 3d % 3d"), powerMode, average_speed_8, average_speed_32);
            u8g2.drawUTF8(0, DISPLAY_HEIGHT - 1, textbuf);
            // Show heartbeat
            uint8_t hbHrs = heartbeat / 7200;
            uint8_t hbMin = ((heartbeat >> 1) % 3600) / 60;
            sprintf_P(textbuf, PSTR("%02d:%02d"), hbHrs, hbMin);
            u8g2.drawUTF8(DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 1, textbuf);

            u8g2.setFont(font_altitude);
            sprintf_P(textbuf, PSTR("%4d"), altitude_to_show);
            u8g2.drawUTF8(0, DISPLAY_HEIGHT - 10, textbuf);
            
        } while ( u8g2.nextPage() );
    }

    INTACK = false;
    if (disable_sleep) {
        // When PWM led control or sound is active, we cannot go into power down mode -
        // it will stop timers and break PWM control.
        // TODO: Try to use Idle mode here keeping timers running, but stopping CPU clock.
        for (;;) {
            if (INTACK)
                break;
        }
    } else {
        off_2s;
    }

    interval_number++;
    if (powerMode == MODE_ON_EARTH || powerMode == MODE_DUMB) {
        // Check auto-poweroff. Prevent it in jump modes.
        heartbeat --;
        if (heartbeat <= 0)
            PowerOff();
    }
}
