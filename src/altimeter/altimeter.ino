#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include "hwconfig.h"
#include "MPL.h"
#include "power.h"
#include "kbd_led.h"
#include "display.h"
#if DISPLAY==DISPLAY_NOKIA5110
#include "fonts/fonts_nokia.h"
#endif
#if DISPLAY==DISPLAY_NOKIA1201
#include "fonts/fonts_hx1230.h"
#endif
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

#include "automatics.h"
//
// State machine sequences
//
//   Dumb) DUMB -> DUMB (Show altitude only; no automatic, no logbook, etc.)
// Normal) PREFILL -> ON_EARTH <-> IN_AIRPLANE <-> EXIT <-> BEGIN_FREEFALL -> FREEFALL <-> PULLOUT <-> OPENING -> UNDER_PARACHUTE -> ON_EARTH

settings_t settings;
jump_profile_t jump_profile;
#ifdef AUDIBLE_SIGNALS_ENABLE
audible_signals_t audible_signals;
#endif

MPL3115A2 myPressure;
Rtc rtc;

uint32_t interval_number; // global heartbeat counter
long heartbeat; // heartbeat in 500ms ticks until auto turn off
int vspeed[32]; // used in mode detector. TODO clean it in setup maybe?
uint8_t vspeedPtr = 0; // pointer to write to vspeed buffer
int current_speed;
int average_speed_8;
int average_speed_32;


// ********* Main altimeter move flag
uint8_t altimeter_mode;
uint8_t previous_altimeter_mode;
volatile bool INTACK;

int current_altitude; // currently measured altitude, relative to ground_altitude
int previous_altitude; // Previously measured altitude, relative to ground_altitude
int altitude_to_show; // Last shown altitude (jitter corrected)
uint16_t batt; // battery voltage as it goes from sensor
int8_t rel_voltage; // ballery relative voltage, in percent (0-100). 0 relates to 3.6v and 100 relates to fully charged batt

#ifdef LOGBOOK_ENABLE
jump_t current_jump;
#endif

char textbuf[48];
char bigbuf[SNAPSHOT_SIZE]; // also used in snapshot writer

uint8_t time_while_btn_menu_pressed = 0;
uint8_t time_while_backlight_on = 0;

static const char* TIME_TMPL;

// Prototypes
#ifdef GREETING_ENABLE
void showText(const uint8_t x, const uint8_t y, const char* text);
#endif
char myMenu(char *menudef, char event = ' ', bool enable_repeat = false);
void showVersion();
void wake() {}
bool SetDate(timestamp_t &date);
#ifdef ALARM_ENABLE
bool checkAlarm();
#endif
void powerOff(bool verbose = true);

void memoryDump();
void showSignals();


void pciSetup(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR (PCINT2_vect) { // handle pin change interrupt for D0 to D5 here (328p)
    INTACK = true;
}

ISR (PCINT1_vect) { // handle pin change interrupt for A0 to A5 here (328p)
}

void loadJumpProfile() {
    EEPROM.get(EEPROM_JUMP_PROFILES + ((settings.jump_profile_number >> 2) * sizeof(jump_profile_t)), jump_profile);
#ifdef AUDIBLE_SIGNALS_ENABLE
    EEPROM.get(EEPROM_AUDIBLE_SIGNALS + ((settings.jump_profile_number) * sizeof(audible_signals_t)), audible_signals);
#endif    
}

void setup() {
    // Configure keyboard and enable pullup resistors
    TIME_TMPL = PSTR("%02d:%02d");
    DDRC &= 0xf0; // PIN_BTN1, PIN_BTN2, PIN_BTN3, PIN_BAT_SENSE => input
    PORTC |= 0x0e; // Pullup on PIN_BTN1, PIN_BTN2, PIN_BTN3
    DDRD = 0xf8; // PIN_INTERRUPT to INPUT, serial to input, rest to output
    PORTD = 0x14; // HWON up, PIN_INTERRUPT to pullup, the rest to 0. Will turn ON display light too.

    Wire.begin();
    delay(50); // Wait hardware to start

    rtc.init(true);
    rtc.enableHeartbeat();  // reset alarm flags, start generate seed sequence
    rtc.readTime();
#ifdef ALARM_ENABLE
    rtc.readAlarm();
#endif

    // Read presets
    altitude_to_show = 0;
    altimeter_mode = previous_altimeter_mode = MODE_PREFILL;
    previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
    vspeedPtr = 0;
    interval_number = 0;

    EEPROM.get(EEPROM_SETTINGS, settings);
    loadJumpProfile();

    if (settings.stored_jumps > settings.total_jumps) {
        // First run => correct
        settings.stored_jumps = settings.stored_snapshots = settings.total_jumps;
        if (settings.stored_snapshots > SNAPSHOT_JOURNAL_SIZE)
            settings.stored_snapshots = SNAPSHOT_JOURNAL_SIZE;
    }

    LED_show(0, 0, 0);

    initSound();

    u8g2.begin();
#if DISPLAY==DISPLAY_NOKIA1201
    u8g2.setContrast((uint8_t)(settings.contrast << 4) + 15);
#endif
//    u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
    u8g2.setDisplayRotation((settings.display_rotation) ? U8G2_R0 : U8G2_R2);

#ifdef ALARM_ENABLE
    if (checkAlarm())
        powerOff(false);
#endif

    //Configure the sensor
    myPressure.begin(); // Get sensor online

    heartbeat = ByteToHeartbeat(settings.auto_power_off);

    if (myPressure.readAltitude() < 450) { // "Reset in airplane" - Disable greeting message if current altitude more than 450m
        DISPLAY_LIGHT_ON;
        Serial.begin(SERIAL_SPEED);
        Serial.println(F("PEGASUS_OK"));
#ifdef GREETING_ENABLE
        SET_MAX_VOL;
        delay(1000);
        memoryDump();
        sound(SIGNAL_WELCOME);
        
        // Show greeting message
        char tmp_buf[16];
#ifdef CUSTOM_GREETING_ENABLE
        uint8_t has_custom_message;
        EEPROM.get(0x1f, has_custom_message);
        if (has_custom_message == 0x41) {
            EEPROM.get(0x130, tmp_buf);
        }
        else
#endif
            strcpy_P(tmp_buf, MSG_HELLO);
        showText(16, 30, tmp_buf);

        DISPLAY_LIGHT_ON;
        showVersion();
        delay(7000);
        DISPLAY_LIGHT_OFF;
#else
        delay(3000); // wait for memory dump request
#endif // GREETING_ENABLE
        termSerial();
    }
    
    // Enable wake by pin-change interrupt from RTC that generates 1Hz 50% duty cycle
    pciSetup(PIN_INTERRUPT_HEARTBEAT);
}

#ifdef ALARM_ENABLE
bool checkAlarm() {
    if (rtc.alarm_enable && rtc.hour == rtc.alarm_hour && rtc.minute == rtc.alarm_minute) {
        rtc.alarm_enable = 0;
        rtc.setAlarm();

        sprintf_P(bigbuf, TIME_TMPL, rtc.hour, rtc.minute);
        u8g2.setFont(font_alarm);
        u8g2.firstPage();
        do {
            u8g2.drawUTF8((DISPLAY_WIDTH - 80) >> 1, (DISPLAY_HEIGHT >> 1) + 12, bigbuf);
        } while(u8g2.nextPage());

        while (BTN2_PRESSED);
        for (uint8_t i = 128; i; ++i) {
#ifdef SOUND_VOLUME_CONTROL_ENABLE
            byte vol = ((i & 127) >> 3);
            if (vol > 3)
                vol = 3;
            setVol(vol);
#endif
            sound(SIGNAL_2SHORT);
            digitalWrite(PIN_LIGHT, i & 1);
            delay(1000);
            if (BTN2_PRESSED)
                break;
        }
        DISPLAY_LIGHT_OFF;
        return true;
    }

    return false;
}
#endif

uint8_t airplane_300m = 0;
void showSignals() {
    if ((MODE_IN_AIR && settings.backlight == 2)
        || (MODE_IN_JUMP && settings.backlight == 3)
        || settings.backlight == 1
        || (time_while_backlight_on > 0 && time_while_btn_menu_pressed < 32))
        DISPLAY_LIGHT_ON;
    else
        DISPLAY_LIGHT_OFF;
    
    // Blink as we entering menu
    switch (time_while_btn_menu_pressed) {
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
    if (altimeter_mode != MODE_ON_EARTH && settings.use_audible_signals && current_altitude > 0 && previous_altitude > 0) {
        for (int i = 7; i >=0; i--) {
            if (current_altitude <= audible_signals.signals[i] && previous_altitude > audible_signals.signals[i]) {
                SET_VOL;
                sound(128 + i);
                break;
            }
        }
    }
#endif            

    switch (altimeter_mode) {
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
}

#ifdef GREETING_ENABLE
void showText(const uint8_t x, const uint8_t y, const char* text) {
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

void powerOff(bool verbose) {
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
        showText(16, 30, tmpBuf);
    }
#endif
    noSound();
    rtc.disableHeartbeat(); // Do it before enabling alarm, because it will tirn alarm off on pcf8583
#ifdef ALARM_ENABLE
    rtc.enableAlarmInterrupt();
#endif

    // turn off i2c
    TWCR &= ~_BV(TWEN);
    // turn off SPI
    SPI.end();
    
    // Stop all on portC, enable pullups on keyboard
    DDRC = 0;
    PORTC = 0x0e;
    
    // Stop all on portB, no pullups
    // TODO: Keep sound amplifier off as well.
    DDRB = 0;
    PORTB = 0;

    // After we turned off all peripherial connections, turn peripherial power OFF too.
    PORTD = 0x84; // LED, sound, HWON to 0, display light to 1 that turns it off, pullup in PIN_INTERRUPT

    // Wake by pin-change interrupt on any key.
    // Alarm, if enabled, already has its pinchange interrupt set.
    pciSetup(PIN_BTN1);
    pciSetup(PIN_BTN2);
    pciSetup(PIN_BTN3);
    termSerial();

    for(;;) {
        powerDown();
        checkWakeCondition();
    }
}

void checkWakeCondition () {
#ifdef ALARM_ENABLE
    if (! (PIND & 0x4)) { // alarm => interrupt => wake
        hardwareReset();
    }
#endif
    uint8_t pin = PINC & 0x0e;
    if (pin != 0x0e) { // A button pressed.
        for (uint8_t i = 0; i < 193; ++i) {
            if ((PINC & 0x0e) != pin) {
                return;
            }
            if (! (i & 63))
                LED_show(0, 0, 80, 200);
            delay(15);
        }
        hardwareReset();
    }
}

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
char myMenu(char *menudef, char event, bool enable_repeat) {
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
        uint8_t key = getKeypress(enable_repeat);
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
    int average_freefall_speed_kmh = (average_freefall_speed_ms * 36) / 10;
    int max_freefall_speed_kmh = (current_jump.max_freefall_speed_ms * 36) / 10;
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
        jump_to_show, settings.total_jumps, getProfileChar(current_jump.profile),
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
        sprintf_P(bigbuf, PSTR(MSG_SETTINGS_SET_SIGNALS_SIGNAL),
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
        event = myMenu(bigbuf, '*', true);
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
            } else {
                if (audible_signals.signals[pos] <= 4995)
                    audible_signals.signals[pos] += 5;
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
                settings.jump_profile_number += 4;
                settings.jump_profile_number &= 0xc;
#endif
                loadJumpProfile();
                break;
            case '0': {
                // Set to zero
                if (altimeter_mode == MODE_ON_EARTH || altimeter_mode == MODE_DUMB) {
                    // Zero reset allowed on earth and in dumb mode only.
                    myPressure.zero();
                    current_altitude = altitude_to_show = 0;
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
                        MSG_LOGBOOK_DELETE_LAST_JUMP
                        MSG_LOGBOOK_CLEAR));
                    logbook_event = myMenu(bigbuf, logbook_event);
                    switch (logbook_event) {
                        case  ' ':
                            // Exit submenu
                            break;
                        case 'V': {
                            // view logbook
                            if (settings.stored_jumps == 0) {
                                LED_show(255, 0, 0, 500);
                                break; // no jumps, nothing to show
                            }
                            uint16_t jump_to_show = settings.total_jumps;
                            // calculate number of first jump to show
                            uint16_t first_stored_jump = settings.total_jumps - settings.stored_jumps;
                            uint8_t logbook_view_event;
                            for (;;) {
                                if (jump_to_show <= first_stored_jump)
                                    jump_to_show = settings.total_jumps;
                                if (jump_to_show > settings.total_jumps)
                                    jump_to_show = first_stored_jump + 1;

                                // Read jump from logbook
                                loadJump(jump_to_show - 1);
                                current_jump_to_bigbuf(jump_to_show);
                                logbook_view_event = (uint8_t)myMenu(bigbuf, '*', true);
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
                            if (settings.stored_jumps == 0) {
                                LED_show(255, 0, 0, 500);
                                break; // no jumps, nothing to show
                            }
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
                                settings.total_jumps = settings.stored_jumps = settings.stored_snapshots = 0;
                                EEPROM.put(EEPROM_SETTINGS, settings);
                                return; // exit logbook menu
                            }
                            break;
                        }

                        case 'D': {
                            if (settings.stored_jumps == 0) {
                                LED_show(255, 0, 0, 500);
                                break; // no jumps, nothing to show
                            }
                            // Delete last jump
                            strcpy_P(bigbuf, PSTR(
                                MSG_LOGBOOK_DELETE_LAST_JUMP_CONFIRM_TITLE
                                MSG_CONFIRM_NO
                                MSG_CONFIRM_YES));
                            char confirmDeleteLastJump = myMenu(bigbuf, ' ');
                            if (confirmDeleteLastJump == 'z')
                                return;
                            if (confirmDeleteLastJump == 'Y') {
                                // Really delete last jump
                                settings.total_jumps--;
                                settings.stored_jumps--;
                                if (settings.stored_snapshots)
                                    settings.stored_snapshots--;
                                EEPROM.put(EEPROM_SETTINGS, settings);
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
                    char power_mode_char = altimeter_mode == MODE_DUMB ? '-' : '~';
                    char zero_after_reset = settings.zero_after_reset ? '~' : '-';
                    textbuf[0] = '\0';
#ifdef ALARM_ENABLE
                    if (rtc.alarm_enable)
                        sprintf_P(textbuf, TIME_TMPL, rtc.alarm_hour, rtc.alarm_minute);
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
                        MSG_SETTINGS_SET_ROUNDING
                        MSG_SETTINGS_SET_AUTO_ZERO
                        MSG_SETTINGS_SET_AUTO_POWER_OFF
                        MSG_SETTINGS_SET_SCREEN_ROTATION
#if DISPLAY==DISPLAY_NOKIA1201
                        MSG_SETTINGS_SET_SCREEN_CONTRAST
#endif
                        MSG_SETTINGS_ABOUT
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
                        precisionMultiplier(),
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
                                    rtc.alarm_enable ? '~' : '-', rtc.alarm_hour, rtc.alarm_minute);
                                alarmEvent = myMenu(bigbuf, alarmEvent);
                                switch(alarmEvent) {
                                    case 'S':
                                        rtc.alarm_enable = !rtc.alarm_enable;
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
                            break;
                        }
#endif
                        case 'Q':
                            // Auto power mode
                            altimeter_mode = altimeter_mode == MODE_DUMB ? MODE_ON_EARTH : MODE_DUMB;
                            break;

                        case 'P':
                            settings.precision_in_freefall++;
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
                powerOff();
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
        uint8_t keyEvent = getKeypress(true);
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

bool SetTime(uint8_t &hour, uint8_t &minute, const char* title) {
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
        uint8_t keyEvent = getKeypress(true);
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
    if (!Serial.available())
        return;
    char cmd = Serial.read();
    if (cmd == '?' || cmd == 'H') {
        sprintf_P(bigbuf, PSTR("\nPEGASUS_BEGIN\nPLATFORM %c%c%c%c\nVERSION " VERSION "\nLANG " LANGUAGE "\nLOGBOOK %c %04x %u\nSNAPSHOT %c %04x %u %u\n"),
        PLATFORM_1, PLATFORM_2, PLATFORM_3, PLATFORM_4,
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
    }
    uint16_t i;
    if (cmd == '?' || cmd == 'R') {
        Serial.print(F("\nROM_BEGIN"));
        for (i = 0; i < 1024; i++) {
            if (! (i & 15)) {
                sprintf_P(textbuf, PSTR("\n%04x:"), i);
                Serial.print(textbuf);
            }
            sprintf_P(textbuf, PSTR(" %02x"), EEPROM.read(i));
            Serial.print(textbuf);
        }
        Serial.print(F("\nROM_END\n"));
    }
#if defined(FLASH_PRESENT)
    if (cmd == '?' || cmd == 'R') {

        Serial.print(F("\nFLASH_BEGIN"));
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
        Serial.print(F("\nFLASH_END\n"));
    }
#endif
    if (cmd == '?' || cmd == 'H')
        Serial.println(F("\nPEGASUS_END"));
}

void loop() {
    // Read sensors
    rtc.readTime();
    current_altitude = myPressure.readAltitude();
    previous_altimeter_mode = altimeter_mode;
    bool refresh_display = (altimeter_mode != MODE_ON_EARTH);

    if (BTN2_PRESSED) {
        time_while_backlight_on = 16;
        // Try to enter menu
        if (time_while_btn_menu_pressed < 32)
            time_while_btn_menu_pressed++;
    } else {
        if (time_while_backlight_on > 0)
            time_while_backlight_on--;
        if (time_while_btn_menu_pressed > 6 && time_while_btn_menu_pressed < 10) {
            if (MODE_IN_JUMP) {
                // Forcibly Save snapshot for debug purposes
#ifdef LOGBOOK_ENABLE
                saveJump();
#endif
                altimeter_mode = MODE_ON_EARTH;
            }
            LED_show(0, 0, 0);
            userMenu();
            refresh_display = true; // force display refresh
            previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
        }
        time_while_btn_menu_pressed = 0;
    }
    
    if ((interval_number & 127) == 0 || altimeter_mode == MODE_PREFILL) {
        // Check and refresh battery meter
        batt = analogRead(PIN_BAT_SENSE);
        if (batt < 450 && altimeter_mode == MODE_PREFILL)
            analogReference(INTERNAL);
        rel_voltage = (int8_t)((batt - settings.batt_min_voltage - 2) * settings.batt_multiplier / 100);
        if (rel_voltage < 0)
            rel_voltage = 0;
        if (rel_voltage > 100)
            rel_voltage = 100;
    }

#ifdef ALARM_ENABLE
    if (altimeter_mode == MODE_ON_EARTH || altimeter_mode == MODE_PREFILL || altimeter_mode == MODE_DUMB)
        checkAlarm();
#endif
    
    processAltitude();
    if (altimeter_mode != previous_altimeter_mode)
        interval_number = 0;

    refresh_display = refresh_display || !(interval_number & 31);
    
    showSignals();

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
    
            if (altimeter_mode == MODE_ON_EARTH) {
#ifdef ALARM_ENABLE
                if (rtc.alarm_enable)
                    sprintf_P(textbuf, PSTR("&%1d' %02d:%02d@"), altimeter_mode, rtc.alarm_hour, rtc.alarm_minute);
                else
//                    sprintf_P(textbuf, PSTR("&%1d' %d"), altimeter_mode, batt);
                    sprintf_P(textbuf, PSTR("&%1d'"), altimeter_mode);
#else            
                sprintf_P(textbuf, PSTR("&%1d'"), altimeter_mode);
#endif
            } else
                sprintf_P(textbuf, PSTR("&%1d' % 3d % 3d"), altimeter_mode, average_speed_8, average_speed_32);
            u8g2.drawUTF8(0, DISPLAY_HEIGHT - 1, textbuf);
            // Show heartbeat
            uint8_t hbHrs = heartbeat / 7200;
            uint8_t hbMin = ((heartbeat >> 1) % 3600) / 60;
            sprintf_P(textbuf, TIME_TMPL, hbHrs, hbMin);
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
        powerDown();
    }

    interval_number++;
    if (altimeter_mode == MODE_ON_EARTH || altimeter_mode == MODE_DUMB) {
        // Check auto-poweroff. Prevent it in jump modes.
        heartbeat --;
        if (heartbeat == 0)
            powerOff();
    }
}
