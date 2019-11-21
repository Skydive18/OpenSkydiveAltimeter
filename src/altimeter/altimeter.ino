#include <EEPROM.h>
#include "power.h"
#include <SPI.h>
#include <Wire.h>
#include "MPL.h"
#include "hwconfig.h"
#include "led.h"
#include "keyboard.h"
#include "display.h"
#ifdef DISPLAY_NOKIA
#include "fonts_nokia.h"
#endif
#ifdef DISPLAY_HX1230
#include "fonts_hx1230.h"
#endif
#include "custom_types.h"
#include "common.h"
#include "PCF8583.h"
#include "snd.h"
#include "logbook.h"
#ifdef FLASH_ENABLE
#include "flash.h"
FlashRom flash;
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
#define EXIT_TH -7
#define BEGIN_FREEFALL_TH -10
#define FREEFALL_TH -40

#define PULLOUT_TH -40
#define OPENING_TH -14
#define UNDER_PARACHUTE_TH -7

settings_t settings;

MPL3115A2 myPressure;
PCF8583 rtc;
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
uint8_t backLight = 0; // Display backlight flag, 0=off, 1=on, 2 = auto
uint16_t batt; // battery voltage as it goes from sensor
int8_t rel_voltage; // ballery relative voltage, in percent (0-100). 0 relates to 3.6v and 100 relates to fully charged batt

// Current jump
jump_t current_jump;
// logbook
uint16_t total_jumps;

// Used in snapshot saver
uint16_t accumulated_altitude;
int accumulated_speed;

char textbuf[48];
char bigbuf[SNAPSHOT_SIZE]; // also used in snapshot writer

uint8_t timeWhileBtnMenuPressed = 0;
uint8_t timeToTurnBacklightOn = 0;

// Prototypes
void ShowText(const uint8_t x, const uint8_t y, const char* text);
char myMenu(char *menudef, char event = ' ');
void showVersion();
void wake() {
    INTACK = true;
}
bool SetDate(timestamp_t &date);
bool checkAlarm();

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)

void pciSetup(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR (PCINT1_vect) { // handle pin change interrupt for A0 to A5 here
}

#endif

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
    delay(20); // Wait hardware to start

    rtc.readTime();
    rtc.readAlarm();

    // Read presets
    altitude_to_show = 0;
    powerMode = previousPowerMode = MODE_PREFILL;
    previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
    vspeedPtr = 0;
    interval_number = 0;
    EEPROM.get(EEPROM_JUMP_COUNTER, total_jumps);
    EEPROM.get(EEPROM_SETTINGS, settings);

    Serial.begin(SERIAL_SPEED); // Console

    LED_show(0, 0, 0);

    initSound();

//  while (!Serial) delay(100); // wait for USB connect, 32u4 only.
/*
  Serial.println("Scanning I2C Bus...");
  int i2cCount = 0;
  for (uint8_t i = 8; i < 128; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      i2cCount++;
      delay (1);  // maybe unneeded?
      } // end of good response
  } // end of for loop
  Serial.print ("Scanning I2C Bus Done. Found ");
  Serial.print (i2cCount, DEC);
  Serial.println (" device(s).");
*/

    u8g2.begin();
#ifdef DISPLAY_HX1230    
    u8g2.setContrast((uint8_t)(settings.contrast << 4) + 15);
#endif
    u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
    u8g2.setDisplayRotation((settings.display_rotation & 1) ? U8G2_R0 : U8G2_R2);

    if (checkAlarm())
        PowerOff();

    //Configure the sensor
    myPressure.begin(); // Get sensor online

    backLight = IIC_ReadByte(RTC_ADDRESS, ADDR_BACKLIGHT);
    heartbeat = ByteToHeartbeat(IIC_ReadByte(RTC_ADDRESS, ADDR_AUTO_POWEROFF));

    // Show greeting message
    ShowText(16, 30, PSTR("Ni Hao!"));
    DISPLAY_LIGHT_ON;
    showVersion();
    delay(4000);
    DISPLAY_LIGHT_OFF;
}

bool checkAlarm() {
    if (rtc.alarm_enable && rtc.hour == rtc.alarm_hour && rtc.minute == rtc.alarm_minute) {
        rtc.alarm_enable = false;
        rtc.setAlarm();

        sprintf_P(bigbuf, PSTR("%02d:%02d"), rtc.hour, rtc.minute);
        u8g2.firstPage();
        do {
            u8g2.drawUTF8(20, 20, bigbuf);
        } while(u8g2.nextPage());

        while (BTN2_PRESSED) delay(100);
        for (uint8_t i = 0; i < 90; ++i) {
            DISPLAY_LIGHT_ON;
            sound(500, 0);
            delay(100);
            noSound();
            delay(100);
            sound(500, 0);
            delay(100);
            noSound();
            delay(200);
            DISPLAY_LIGHT_OFF;
            delay(500);
            if (BTN2_PRESSED)
                break;
        }
        return true;
    }

    return false;
}

void saveToJumpSnapshot() {
    if (current_jump.total_jump_time < 2045)
        current_jump.total_jump_time++;

    if (current_jump.total_jump_time > 1198 || (current_jump.total_jump_time & 1) /* write once per second */ )
        return;
    int speed_now = current_altitude - accumulated_altitude;
    // TODO. Limit acceleration (in fact, this is exactly what we store) with values depending on current speed.
    // Anyway, while freefall, acceleration more than 1g is not possible.
    bigbuf[current_jump.total_jump_time >> 1] = (char)(speed_now - accumulated_speed);
    accumulated_altitude = current_altitude;
    accumulated_speed = speed_now;
}

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
    previous_altitude = current_altitude;
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
            if (settings.zero_after_reset & 1) {
                altitude_to_show = 0;
                myPressure.zero();
            }
            powerMode = MODE_ON_EARTH;
            return;
        }
    }

    //
    // ************************
    // ** MAIN STATE MACHINE **
    // ************************
    //
    // Due to compiler optimization, **DO NOT** use `case` here.
    // Also, **DO NOT** remove `else`'s.

    if (powerMode == MODE_ON_EARTH) {
        if ( (current_altitude > 250) || (current_altitude > 30 && average_speed_8 > 1) )
            powerMode = MODE_IN_AIRPLANE;
        else if (average_speed_8 < -25) {
            powerMode = MODE_FREEFALL;
            current_jump.total_jump_time = 2047; // do not store this jump in logbook
        }
    } else
    if (powerMode == MODE_IN_AIRPLANE) {
        if (current_speed <= EXIT_TH) {
            powerMode = MODE_EXIT;
            current_jump.exit_altitude = (accumulated_altitude = current_altitude) >> 1;
            current_jump.exit_timestamp = rtc.getTimestamp();
            current_jump.total_jump_time = current_jump.max_freefall_speed_ms = 0;
            bigbuf[0] = (char)current_speed;
            accumulated_speed = current_speed;
        }
        else if (average_speed_32 < 1 && current_altitude < 25)
            powerMode = MODE_ON_EARTH;
    } else
    if (powerMode == MODE_EXIT) {
        saveToJumpSnapshot();
        if (current_speed <= BEGIN_FREEFALL_TH) {
            powerMode = MODE_BEGIN_FREEFALL;
        }
        else if (current_speed > EXIT_TH)
            powerMode = MODE_IN_AIRPLANE;
    } else
    if (powerMode == MODE_BEGIN_FREEFALL) {
        saveToJumpSnapshot();
        if (current_speed <= FREEFALL_TH)
            powerMode = MODE_FREEFALL;
        else if (current_speed > BEGIN_FREEFALL_TH)
            powerMode = MODE_EXIT;
    } else
    if (powerMode == MODE_FREEFALL) {
        saveToJumpSnapshot();
        if (current_speed > PULLOUT_TH)
            powerMode = MODE_PULLOUT;
        current_jump.deploy_altitude = current_altitude >> 1;
        current_jump.deploy_time = current_jump.total_jump_time;
        int8_t freefall_speed = 0 - average_speed_8;
        if (freefall_speed > current_jump.max_freefall_speed_ms)
            current_jump.max_freefall_speed_ms = freefall_speed;
    } else
    if (powerMode == MODE_PULLOUT) {
        saveToJumpSnapshot();
        if (current_speed <= PULLOUT_TH)
            powerMode = MODE_FREEFALL;
        else if (current_speed > OPENING_TH) {
            powerMode = MODE_OPENING;
        }
    } else
    if (powerMode == MODE_OPENING) {
        saveToJumpSnapshot();
        if (current_speed <= OPENING_TH)
            powerMode = MODE_PULLOUT;
        else if (current_speed > UNDER_PARACHUTE_TH) {
            powerMode = MODE_UNDER_PARACHUTE;
            current_jump.canopy_altitude = current_jump.deploy_altitude - (current_altitude >> 1);
        }
    } else
    if (powerMode == MODE_UNDER_PARACHUTE) {
        saveToJumpSnapshot();
        if (average_speed_32 > -1 && current_altitude < 25) {
            powerMode = MODE_ON_EARTH;
            // try to lock zero altitude
            altitude_to_show = 0;
            // Save snapshot
            EEPROM.put(SNAPSHOT_START, bigbuf);
            // Save jump
            EEPROM.put(EEPROM_LOGBOOK_START +  (((total_jumps++) % (LOGBOOK_SIZE + 1)) * sizeof(jump_t)), current_jump);
            EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
        }
    }
    
}

uint8_t airplane_300m = 0;
void ShowLEDs() {
    if ((powerMode != MODE_ON_EARTH && powerMode != MODE_PREFILL && powerMode != MODE_DUMB && backLight == 2) || backLight == 1 || timeToTurnBacklightOn > 0)
        DISPLAY_LIGHT_ON;
    else
        DISPLAY_LIGHT_OFF;
    
    // Blink as we entering menu
    switch (timeWhileBtnMenuPressed) {
        case 0:
        case 14:
        case 15:
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
        default: // 2, 4, 6, 8, 9, 11, 13 => off all LEDs
            LED_show(0, 0, 0);
            return;
    }
    
    switch (powerMode) {
        case MODE_IN_AIRPLANE:
            if (current_altitude >= 300) {
                if (airplane_300m <= 6 && current_altitude < 900) {
                    LED_show(0, ((++airplane_300m) & 1) ? 140 : 0, 0); // green blinks 3 times at 300m but not above 900m
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
            return;
        case MODE_EXIT:
        case MODE_BEGIN_FREEFALL:
        case MODE_FREEFALL:
        case MODE_PULLOUT:
        case MODE_OPENING:
            if (current_altitude < 1000) {
                LED_show(255, 0, 0); // red ligth below 1000m
            } else if (current_altitude < 1200) {
                LED_show(255, 80, 0); // yellow ligth between 1200 and 1000m
            } else if (current_altitude < 1550) {
                LED_show(0, 255, 0); // green ligth between 1550 and 1200m
            } else {
                LED_show(0, 0, 255); // blue ligth above 1550m
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
                    sound(700, 0);
                }
            } else
                break; // turn LED off
            return;
    }

    airplane_300m = 0;
    LED_show(0, 0, 0);
    noSound();
}

void ShowText(const uint8_t x, const uint8_t y, const char* text) {
    u8g2.setFont(font_hello);
    DISPLAY_LIGHT_ON;
    uint8_t maxlen = strlen_P(text);
    for (uint8_t i = 1; i <= maxlen; i++) {
        strcpy_P(bigbuf, text);
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

void PowerOff() {
    ShowText(6, 24, PSTR("Sayonara"));

    rtc.disableSeedInterrupt();
  
    termSound();

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
    
    do {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
        // Wake by pin-change interrupt on any key
        pciSetup(PIN_BTN1);
        pciSetup(PIN_BTN2);
        pciSetup(PIN_BTN3);
#endif
#if defined(__AVR_ATmega32U4__)
        // Wake by BTN2 (Middle button)
        attachInterrupt(digitalPinToInterrupt(PIN_BTN2), wake, LOW);
#endif
        // Also wake by alarm. TODO.
//        attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
        off_forever;
//        detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
#if defined(__AVR_ATmega32U4__)
        detachInterrupt(digitalPinToInterrupt(PIN_BTN2));
#endif
    } while (!checkWakeCondition());
    resetFunc();
}

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
uint8_t checkWakeCondition () {
    uint8_t pin;
    if (BTN1_PRESSED)
        pin = PIN_BTN1;
    else if (BTN2_PRESSED)
        pin = PIN_BTN2;
    else if (BTN3_PRESSED)
        pin = PIN_BTN3;
    else
        return 0; // todo: check if alarm fired
    // Determine wake condition
    LED_show(0, 0, 80, 400);
    for (uint8_t i = 1; i < 193; ++i) {
        if (digitalRead(pin))
            return 0;
        if (! (i & 63))
            LED_show(0, 0, 80, 200);
        off_15ms;
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
        if (BTN1_RELEASED)
            return 0;
        if (! (i & 63))
            LED_show(0, 0, 80, 200);
        off_15ms;
        }
        return 1;
    }
    // todo: check if alarm fired
    return 0;
}
#endif

void showVersion() {
    u8g2.setFont(font_menu);
    sprintf_P(bigbuf, PSTR(
        "Альтимонстр I\n"
#if defined(__AVR_ATmega32U4__)
        "Платформа A01\n"
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
        "Платформа B01\n"
#endif
        "Версия 0.90\n"
        "COM %ld/8N1\n"),
        SERIAL_SPEED);
    myMenu(bigbuf, 'z');
}

// Returns ' ' on timeout; otherwise, current position
// event = '\0' => show menu and wait for keypress, no navigation performed.
// event == 'z' => show menu and exit immediately.
char myMenu(char *menudef, char event = ' ') {
    uint16_t ptr = 0;
    char ptrs[12];
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
        if (event == '\0')
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

    sprintf_P(bigbuf, PSTR("%u/%u\n%02d.%02d.%04d %02d:%02d\nO:%4dм Р:%4dм\nП:%4dм В:%4dc\nС:%dм/с %dкм/ч\nМ:%dм/с %dкм/ч\n"),
        jump_to_show, total_jumps,
        day, month, year, hour, minute,
        exit_altitude, deploy_altitude,
        canopy_altitude, freefall_time,
        average_freefall_speed_ms, average_freefall_speed_kmh,
        max_freefall_speed_ms, max_freefall_speed_kmh
        );
}

void userMenu() {
    DISPLAY_LIGHT_ON; // it will be turned off, if needed, in ShowLEDs(..)

    u8g2.setFont(font_menu);

    char event = ' ';
    do {
        char bl_char = '~';
        if (backLight == 0)
            bl_char = '-';
        if (backLight == 2)
            bl_char = '*';
        sprintf_P(bigbuf, PSTR(
            "Меню\n"
            " Выход\n"
            "0Сброс на ноль\n"
            "BПодсветка: %c\n"
            "LЖурнал прыжков\n"
//            "AСигналы\n"
            "SНастройки\n"
            "XВыключение\n"),
            bl_char);
        event = myMenu(bigbuf, event);
        switch (event) {
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
                backLight++;
                if (backLight > 2)
                    backLight = 0;
                if (backLight != 1)
                    IIC_WriteByte(RTC_ADDRESS, ADDR_BACKLIGHT, backLight);
                break;
            case 'L': {
                // Logbook
                char logbook_event = ' ';
                do {
                    strcpy_P(bigbuf, PSTR(
                        "Журнал прыжков\n"
                        " Выход\n"
                        "VПросмотр\n"
                        "RПовтор прыжка\n"
                        "CОчистить журнал\n"));
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
                                EEPROM.get(EEPROM_LOGBOOK_START +  (((jump_to_show - 1) % (LOGBOOK_SIZE + 1)) * sizeof(jump_t)), current_jump);
                                current_jump_to_bigbuf(jump_to_show);
                                logbook_view_event = (uint8_t)myMenu(bigbuf, '\0');
                                if (logbook_view_event == PIN_BTN1)
                                    jump_to_show--;
                                if (logbook_view_event == PIN_BTN2)
                                    break;
                                if (logbook_view_event == PIN_BTN3)
                                    jump_to_show++;
                                if (logbook_view_event == ' ')
                                    return;;
                            };
                            break;
                        }
                        case 'R':
                            // replay latest jump
//                            myPressure.debugPrint();
//                            return;
                            break; // not implemented
                            
                        case 'C': {
                            // Erase logbook
                            strcpy_P(bigbuf, PSTR(
                                "Очистить журнал?\n"
                                " Нет\n"
                                "YДа\n"));
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
            case 'S': {
                // "Settings" submenu
                char eventSettings = 1;
                uint8_t newAutoPowerOff = IIC_ReadByte(RTC_ADDRESS, ADDR_AUTO_POWEROFF);
                do {
                    char power_mode_char = powerMode == MODE_DUMB ? '-' : '~';
                    char zero_after_reset = settings.zero_after_reset & 1 ? '~' : '-';
                    sprintf_P(bigbuf, PSTR(
                        "Настройки\n"
                        " Выход\n"
                        "TВремя\n"
                        "DДата\n"
//                        "AБудильник\n"
                        "QАвтоматика: %c\n"
                        "OАвтовыкл: %dч\n"
                        "RПоворот экрана\n"
#if defined(DISPLAY_HX1230)
                        "CКонтраст %d\n"
#endif
                        "0Авто ноль: %c\n"
                        "VВерсия ПО\n"
                        "UДамп памяти\n"),
                        power_mode_char,
                        HeartbeatValue(newAutoPowerOff),
#if defined(DISPLAY_HX1230)
                        settings.contrast & 15,
#endif
                        zero_after_reset);
                    eventSettings = myMenu(bigbuf, eventSettings);
                    switch (eventSettings) {
                        case ' ':
                            break; // Exit menu
                        case 'T': {
                            // Time
                            rtc.readTime();
                            uint8_t hour = rtc.hour;
                            uint8_t minute = rtc.minute;
                            if (SetTime(hour, minute)) {
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
//                        case 'A':
//                            // Alarm
//                            break;
                        case 'Q':
                            // Auto power mode
                            powerMode = powerMode == MODE_DUMB ? MODE_ON_EARTH : MODE_DUMB;
                            break;
                        case 'O': {
                            // Auto poweroff
                            newAutoPowerOff = (++newAutoPowerOff) & 3;
                            IIC_WriteByte(RTC_ADDRESS, ADDR_AUTO_POWEROFF, newAutoPowerOff);
                            heartbeat = ByteToHeartbeat(newAutoPowerOff);
                            break;
                        }
                        case 'R':
                            // Screen rotate
                            settings.display_rotation++;
                            u8g2.setDisplayRotation((settings.display_rotation & 1) ? U8G2_R0 : U8G2_R2);
                            break;
#if defined(DISPLAY_HX1230)
                        case 'C':
                            // Contrast
                            settings.contrast++;
                            settings.contrast &= 15;
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
                        case 'U': {
                            uint16_t addr;
                            Serial.print(F("\n:DATA"));
                            for (uint16_t i = 1263; i >= 0; i--) {
                                addr = 1263 - i;
                                uint8_t b = addr < 1024 ? EEPROM.read(addr) : IIC_ReadByte(RTC_ADDRESS, addr - 1008);
                                sprintf_P(textbuf, PSTR(" %02x"), b);
                                Serial.print(textbuf);
                            }
                            Serial.print(F("\n:END"));
                            break;
                        }
                        default:
                            // timeout
                            return;
                    }
                    EEPROM.put(EEPROM_SETTINGS, settings);

                } while (eventSettings != 1);
                break;
            }
            case 'X':
                // Power off
                PowerOff();
                break;
            default:
                return;
        }
    } while (event != ' ' && event != 'z');
}

bool SetDate(uint8_t &day, uint8_t &month, uint16_t &year) {
    uint8_t pos = 0;
    for(;;) {
        pos = (pos + 5) % 5;
        if (year > 2100)
            year = 2100;
        if (year < 2019)
            year = 2019;
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
            sprintf_P(textbuf, PSTR("Текущая дата"));
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

            sprintf_P(textbuf, PSTR("%cОтмена"),
                pos == 3 ? '}' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 5, textbuf);
            
            sprintf_P(textbuf, PSTR("%cОК"),
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

bool SetTime(uint8_t &hour, uint8_t &minute) {
    uint8_t pos = 0;
    for(;;) {
        pos &= 3;
        u8g2.firstPage();
        do {
            strcpy_P(textbuf, PSTR("Текущее время"));
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT, textbuf);
            u8g2.drawHLine(0, MENU_FONT_HEIGHT + 1, DISPLAY_WIDTH-1);

            sprintf_P(textbuf, PSTR("%c%02d:%02d%c"),
                pos == 0 ? '}' : ' ',
                hour,
                minute,
                pos == 1 ? '{' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 3, textbuf);

            sprintf_P(textbuf, PSTR("%cОтмена"),
                pos == 2 ? '}' : ' '
            );
            u8g2.drawUTF8(0, MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + MENU_FONT_HEIGHT + 5, textbuf);
            
            sprintf_P(textbuf, PSTR("%cОК"),
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
        if (timeWhileBtnMenuPressed < 16)
            timeWhileBtnMenuPressed++;
    } else {
        if (timeToTurnBacklightOn > 0)
            timeToTurnBacklightOn--;
        if (timeWhileBtnMenuPressed == 8 || timeWhileBtnMenuPressed == 9) {
            if (BTN3_PRESSED) {
                // Forcibly Save snapshot for debug purposes
                EEPROM.put(SNAPSHOT_START, bigbuf);
                // Save jump
                EEPROM.put(EEPROM_LOGBOOK_START +  (((total_jumps++) % (LOGBOOK_SIZE + 1)) * sizeof(jump_t)), current_jump);
                EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
            }
            userMenu();
            refresh_display = true; // force display refresh
            previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
        }
        timeWhileBtnMenuPressed = 0;
    }
    
    if ((interval_number & 63) == 0 || powerMode == MODE_PREFILL) {
        // Check and refresh battery meter
        batt = analogRead(PIN_BAT_SENSE);
        rel_voltage = (int8_t)((batt - settings.battGranulationD) * settings.battGranulationF);
        if (rel_voltage < 0)
            rel_voltage = 0;
        if (rel_voltage > 100)
            rel_voltage = 100;
        if (powerMode == MODE_ON_EARTH || powerMode == MODE_PREFILL || powerMode == MODE_DUMB)
            checkAlarm();
    }
    
    processAltitude();
    if (powerMode != previousPowerMode)
        interval_number = 0;

    refresh_display += !(interval_number & 31);
    
    ShowLEDs();

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
    
            if (powerMode == MODE_ON_EARTH)
                sprintf_P(textbuf, PSTR("&%1d'"), powerMode);
            else
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

    rtc.enableSeedInterrupt();

    attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
    INTACK = false;
    if (disable_sleep) {
        // When PWM led control or sound delay is active, we cannot go into power down mode -
        // it will stop timers and break PWM control.
        // TODO: Try to use Idle mode here keeping timers running, but stopping CPU clock.
        for (int intcounter = 0; intcounter < 800; ++intcounter) {
            if (INTACK)
                break;
            delay(5);
        }
    } else {
        off_4s;
    }
    detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    rtc.disableSeedInterrupt();

    interval_number++;
    if (powerMode == MODE_ON_EARTH || powerMode == MODE_DUMB) {
        // Check auto-poweroff. Prevent it in jump modes.
        heartbeat --;
        if (heartbeat <= 0)
            PowerOff();
    }
}
