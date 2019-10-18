#include <EEPROM.h>
#include "powerctl.h"
#include <SPI.h>
#include <Wire.h>
#include "osaMPL3115A2.h"
#include "hwconfig.h"
#include "led.h"
#include "keyboard.h"
#include "display.h"
#include "fonts.h"
#include "custom_types.h"
#include "common.h"
#include "PCF8583.h"

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
#define FREEFALL_TH -15

#define PULLOUT_TH -25
#define OPENING_TH -18
#define UNDER_PARACHUTE_TH -12

MPL3115A2 myPressure;
PCF8583 rtc;
void(* resetFunc) (void) = 0;

#define CLEAR_PREVIOUS_ALTITUDE -30000
#define VSPEED_LENGTH 32
int vspeed[VSPEED_LENGTH]; // used in mode detector. TODO clean it in setup maybe?
uint32_t bstep; // global heartbeat counter, in ticks depending on mode
uint32_t bstepInCurrentMode; // heartbeat counter since mode changed
long heartbeat; // heartbeat in 500ms ticks

byte vspeedPtr = 0; // pointer to write to vspeed buffer
int currentVspeed;
int averageSpeed8;
int averageSpeed32;

// ********* Main power move flag
byte powerMode;
byte previousPowerMode;
volatile bool INTACK;

int current_altitude; // currently measured altitude, relative to ground_altitude
int previous_altitude; // Previously measured altitude, relative to ground_altitude
int ground_altitude; // Ground altitude, absolute
int last_shown_altitude; // Last shown altitude (jitter corrected)
byte backLight = 0; // Display backlight flag, 0=off, 1=on, 2 = auto
int batt; // battery voltage as it goes from sensor
int8_t rel_voltage; // ballery relative voltare, in percent (0-100). 0 relates to 3.6v and 100 relates to fully charged batt

// Current jump
jump_t current_jump;

// Used in snapshot saver
uint16_t accumulated_altitude;
int accumulated_speed;

// logbook
uint16_t total_jumps;

char smallbuf[24];
char middlebuf[48];
char bigbuf[SNAPSHOT_SIZE]; // also used in snapshot writer
char* date_time_template;

// Prototypes
void ShowText(const byte x, const byte y, const char* text);
int8_t myMenu(char *menudef, int8_t event = 1);
void showVersion();
void wake() { INTACK = true; }

void setup() {
    pinMode(PIN_HWPWR, OUTPUT);
    Serial.begin(SERIAL_SPEED); // Console

/*
  while (!Serial) delay(100); // wait for USB connect, 32u4 only.
  Serial.println("Scanning I2C Bus...");
  int i2cCount = 0;
  for (byte i = 8; i > 0; i++)
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

    LED_show(0, 0, 0);

    pinMode(PIN_SOUND, OUTPUT);
    digitalWrite(PIN_SOUND, 0);

    // Turn ON hardware
    digitalWrite(PIN_HWPWR, 1);
    
    rtc.begin();
    u8g2.begin(PIN_BTN2, PIN_BTN3, PIN_BTN1);
    u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
    pinMode(PIN_LIGHT, OUTPUT);

    //Configure the sensor
    myPressure.begin(); // Get sensor online

    // Configure keyboard
    pinMode(PIN_BTN1, INPUT_PULLUP);
    pinMode(PIN_BTN2, INPUT_PULLUP);
    pinMode(PIN_BTN3, INPUT_PULLUP);

    pinMode(PIN_INTERRUPT, INPUT_PULLUP);

    // Read presets
    last_shown_altitude = 0;
    powerMode = previousPowerMode = MODE_PREFILL;
    previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
    vspeedPtr = 0;
    bstepInCurrentMode = 0;
    ground_altitude = IIC_ReadInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE);
    backLight = IIC_ReadByte(RTC_ADDRESS, ADDR_BACKLIGHT);
    heartbeat = ByteToHeartbeat(IIC_ReadByte(RTC_ADDRESS, ADDR_AUTO_POWEROFF));
    // setup
//    total_jumps = 0;
//    EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
    EEPROM.get(EEPROM_JUMP_COUNTER, total_jumps);

    strcpy(bigbuf, ""); // need it to correctly allocate memory

    // Show greeting message
    ShowText(16, 30, PSTR("Ni Hao!"));
    DISPLAY_LIGHT_ON;
    showVersion();
    delay(4000);
    DISPLAY_LIGHT_OFF;

    // Load templates
    date_time_template = PSTR("%02d.%02d.%04d %02d%c%02d");
    bstep = 0;
}

void saveToJumpSnapshot() {
    if (current_jump.total_jump_time < 2045)
        current_jump.total_jump_time++;

    if (current_jump.total_jump_time > 1198 || (current_jump.total_jump_time & 1))
        return;
    int speed_now = current_altitude - accumulated_altitude;
    bigbuf[current_jump.total_jump_time >> 1] = (char)(speed_now - accumulated_speed);
    accumulated_altitude = current_altitude;
    accumulated_speed = speed_now;
}

//
// Main state machine
// Ideas got from EZ430 project at skycentre.net
//
void processAltitudeChange(bool speed_scaler) {
    if (previous_altitude == CLEAR_PREVIOUS_ALTITUDE) { // unknown delay or first run; just save
        previous_altitude = current_altitude;
        return;
    }
    
    currentVspeed = current_altitude - previous_altitude; // negative = fall, positive = climb
    previous_altitude = current_altitude;
    if (speed_scaler)
        currentVspeed *= 2; // 500ms granularity
    else
        currentVspeed /= 4; // 4s granularity
    vspeed[vspeedPtr & (VSPEED_LENGTH - 1)] = (int8_t)currentVspeed;
    vspeedPtr++;
    byte startPtr = (byte)((vspeedPtr - VSPEED_LENGTH)) & (VSPEED_LENGTH - 1);
    int accumulatedVspeed = 0;
    for (byte i = 0; i < VSPEED_LENGTH; i++) {
        accumulatedVspeed += vspeed[(startPtr + i) & (VSPEED_LENGTH - 1)];
        if (i == 8)
            averageSpeed8 = accumulatedVspeed / 8;
    }
    averageSpeed32 = accumulatedVspeed / VSPEED_LENGTH;

    switch (powerMode) {
        case MODE_DUMB:
            // No logic in this mode. Manual change only.
            break;
        
        case MODE_PREFILL:
            // Change mode to ON_EARTH if vspeed had been prefilled (approx. 16s after enter to this mode)
            if (vspeedPtr > VSPEED_LENGTH)
                powerMode = MODE_ON_EARTH;
            break;
        
        case MODE_ON_EARTH:
            // Altitude granularity is 4s
            if ( (current_altitude > 250) || (current_altitude > 30 && averageSpeed8 > 1) )
                powerMode = MODE_IN_AIRPLANE;
            else if (averageSpeed8 < -25) {
                powerMode = MODE_FREEFALL;
                current_jump.total_jump_time = 2047; // do not store this jump in logbook
            }
            break;

        case MODE_IN_AIRPLANE:
            if (currentVspeed <= EXIT_TH)
                powerMode = MODE_EXIT;
            else if (averageSpeed32 < 1 && current_altitude < 25)
                powerMode = MODE_ON_EARTH;
            break;

        case MODE_EXIT:
            if (currentVspeed <= BEGIN_FREEFALL_TH)
                powerMode = MODE_BEGIN_FREEFALL;
            else if (currentVspeed > EXIT_TH)
                powerMode = MODE_IN_AIRPLANE;
            else
                break;
            current_jump.exit_altitude = (accumulated_altitude = current_altitude) / 2;
            current_jump.exit_timestamp = rtc.getTimestamp();
            current_jump.total_jump_time = current_jump.max_freefall_speed_ms = 0;
            bigbuf[0] = (char)currentVspeed;
            accumulated_speed = currentVspeed;
            break;

        case MODE_BEGIN_FREEFALL:
            saveToJumpSnapshot();
            if (currentVspeed <= FREEFALL_TH)
                powerMode = MODE_FREEFALL;
            else if (currentVspeed > BEGIN_FREEFALL_TH)
                powerMode = MODE_EXIT;
            else
                break;
            break;

        case MODE_FREEFALL:
            saveToJumpSnapshot();
            if (currentVspeed > PULLOUT_TH)
                powerMode = MODE_PULLOUT;
            else
                break;
            byte freefall_speed = 0 - averageSpeed8;
            if (freefall_speed > current_jump.max_freefall_speed_ms)
                current_jump.max_freefall_speed_ms = freefall_speed;
            break;

        case MODE_PULLOUT:
            saveToJumpSnapshot();
            if (currentVspeed <= PULLOUT_TH) {
                powerMode = MODE_FREEFALL;
            }
            else if (currentVspeed > OPENING_TH)
                powerMode = MODE_OPENING;
            else
                break;
            current_jump.deploy_altitude = current_altitude / 2;
            current_jump.deploy_time = current_jump.total_jump_time;
            break;

        case MODE_OPENING:
            saveToJumpSnapshot();
            if (currentVspeed <= OPENING_TH)
                powerMode = MODE_PULLOUT;
            else if (currentVspeed > UNDER_PARACHUTE_TH) {
                powerMode = MODE_UNDER_PARACHUTE;
                current_jump.canopy_altitude = current_jump.deploy_altitude - (current_altitude / 2);
            }
            break;

        case MODE_UNDER_PARACHUTE:
            saveToJumpSnapshot();
            if (averageSpeed32 < 1 && current_altitude < 25) {
                powerMode = MODE_ON_EARTH;
                // Save snapshot
                EEPROM.put(SNAPSHOT_START, bigbuf);
                // Save jump
                EEPROM.put(EEPROM_LOGBOOK_START + (((total_jumps++) * sizeof(jump_t)) % LOGBOOK_SIZE), current_jump);
                EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
            }
            break;
    }
}

byte airplane_300m = 0;
void ShowLEDs(bool powerModeChanged, byte timeWhileBtn1Pressed) {
    if ((powerMode != MODE_ON_EARTH && powerMode != MODE_PREFILL && powerMode != MODE_DUMB && backLight == 2) || backLight == 1)
        DISPLAY_LIGHT_ON;
    else
        DISPLAY_LIGHT_OFF;
    
    // Blink as we entering menu
    switch (timeWhileBtn1Pressed) {
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
                    LED_show(0, ((++airplane_300m) & 1) ? 255 : 0, 0); // green blinks 3 times at 300m but not above 900m
                } else {
                    if (current_altitude > 900) {
                        LED_show(0, (bstep & 15) ? 0 : 255, 0); // green blinks once per 8s above 900m
                    } else {
                        LED_show((bstep & 15) ? 0 : 255, (bstep & 15) ? 0 : 80, 0); // yellow blinks once per 8s between 300m and 900m
                    }
                }
            } else {
                LED_show((bstep & 15) ? 0 : 255, 0, 0); // red blinks once per 8s below 300m
            }
            return;
        case MODE_FREEFALL:
            if (current_altitude < 1000) {
                LED_show(255, 0, 0); // red ligth below 1000m
            } else if (current_altitude < 1200) {
                LED_show(255, 160, 0); // yellow ligth between 1200 and 1000m
            } else if (current_altitude < 1550) {
                LED_show(0, 255, 0); // green ligth between 1550 and 1200m
            } else {
                LED_show(0, 0, 255); // blue ligth above 1550m
            }
            return;

        case MODE_PREFILL:
            LED_show((bstep & 3) ? 0 : 255, 0, 0); // red blinks once per 2s until prefill finished
            return;

        case MODE_ON_EARTH:
            if (bstepInCurrentMode < 8) {
                LED_show(0, (bstepInCurrentMode & 1) ? 0 : 80, 0); // green blinks to indicate that altimeter is ready
            }
            return;
    }

    airplane_300m = 0;
    LED_show(0, 0, 0);
}

void ShowText(const byte x, const byte y, const char* text) {
    u8g2.setFont(u8g2_font_helvB10_tr);
    DISPLAY_LIGHT_ON;
    byte maxlen = strlen_P(text);
    for (byte i = 1; i <= maxlen; i++) {
        strcpy_P(bigbuf, text);
        bigbuf[i] = 0;
        u8g2.firstPage();
        do {
            u8g2.setCursor(x, y);
            u8g2.print(bigbuf);
        } while(u8g2.nextPage());
        delay(250);
    }
    delay(5000);
    DISPLAY_LIGHT_OFF;
}

void PowerOff() {
    ShowText(6, 24, PSTR("Sayonara"));

    rtc.disableSeedInterrupt();
  
    digitalWrite(PIN_HWPWR, 0);
    digitalWrite(PIN_SOUND, 0);
    // turn off i2c
    pinMode(SCL, INPUT);
    pinMode(SDA, INPUT);
    // turn off SPI
    pinMode(MOSI, INPUT);
    pinMode(MISO, INPUT);
    pinMode(SCK, INPUT);
    pinMode(SS, INPUT);
#ifdef DC_PIN    
    pinMode(DC_PIN, INPUT);
#endif
    do {
        attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
        off_forever;
        detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    } while (!checkWakeCondition());
    resetFunc();
}

byte checkWakeCondition ()
{
    // Determine wake condition
    if (BTN1_PRESSED) {
        // Wake by awake button. Should be kept pressed for 3s
        LED_show(0, 0, 80, 400);
        for (byte i = 1; i < 193; ++i) {
        if (BTN1_RELEASED)
            return 0;
        if (! (i & 63))
            LED_show(0, 0, 80, 200);
        off_15ms;
        }
        return 1;
    }

    return 0;
}

void showVersion() {
    u8g2.setFont(font_menu);
    sprintf_P(bigbuf, PSTR("Альтимонстр I\nПлатформа A01\nВерсия 0.5\nCOM %ld/8N1\n"), SERIAL_SPEED);
    myMenu(bigbuf, -1);
}

#define FONT_HEIGHT 7
int8_t myMenu(char *menudef, int8_t event = 1) {
    int ptr = 0;
    int8_t number_of_menu_lines = -1;
    while(menudef[ptr]) {
        if (menudef[ptr] == '\n') {
            menudef[ptr] = '\0';
            number_of_menu_lines++;
        }
        ptr++;
    }
    
    int8_t firstline = 1;

    for (;;) {
        // check if we need to update scroll
        if (event > 0) {
            if ((event - firstline) >= DISPLAY_LINES_IN_MENU)
                firstline = event - (DISPLAY_LINES_IN_MENU - 1);
            if (event < firstline)
                firstline = event;
        }
        u8g2.firstPage();
        do {
            ptr = 0;
            int8_t line = 1;
            // Print title and find ptr
            u8g2.setCursor(0, FONT_HEIGHT);
            u8g2.print(menudef);
            while (menudef[ptr++]); // set to the beginning of the next line
            u8g2.drawHLine(0, FONT_HEIGHT + 1, DISPLAY_WIDTH-1);
            while (menudef[ptr] != 0 && (line - firstline) < DISPLAY_LINES_IN_MENU) {
                if (line >= firstline) {
                    u8g2.setCursor(0, (FONT_HEIGHT * (line - firstline)) + (FONT_HEIGHT + FONT_HEIGHT + 2));
                    if (event > 0)
                        menudef[ptr] = (event == line) ? '}' : ' ';
                    u8g2.print((char*)(menudef + ptr));
                }
                line++;
                while (bigbuf[ptr++]); // set to the beginning of the next line
            }
        } while (u8g2.nextPage());
        if (event == -1)
            return -1;
        byte key = getKeypress();
        if (event == 0)
            return key;
        switch (key) {
            case 255:
                // Timeout
                return -1;
            case PIN_BTN2:
                // Select
                return event;
            case PIN_BTN1:
                // up
                event--;
                if (event == 0)
                    event = number_of_menu_lines;
                break;
            case PIN_BTN3:
                // down
                event++;
                if (event > number_of_menu_lines)
                    event = 1;
                break;
        }
    }
}

void userMenu() {
    DISPLAY_LIGHT_ON; // it will be turned off, if needed, in ShowLEDs(..)

    u8g2.setFont(font_menu);

    int8_t event = 1;
    do {
        switch (backLight) {
            case 0: strcpy_P(smallbuf, PSTR("выкл")); break;
            case 2: strcpy_P(smallbuf, PSTR("авто")); break;
            default: strcpy_P(smallbuf, PSTR("вкл")); break;
        }
        sprintf_P(bigbuf, PSTR("Меню\n Выход\n Сброс на ноль\n Подсветка: %s\n Журнал прыжков\n Сигналы\n Настройки\n Выключение\n"), smallbuf);
        event = myMenu(bigbuf, event);
        switch (event) {
            case 2: {
                // Set to zero
                if (powerMode == MODE_ON_EARTH || powerMode == MODE_PREFILL || powerMode == MODE_DUMB) {
                    // Zero reset allowed on earth, in prefill and in dumb mode only.
                    ground_altitude = current_altitude;
                    IIC_WriteInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE, ground_altitude);
                    last_shown_altitude = 0;
                    LED_show(0, 80, 0, 250);
                    return;
                } else {
                    LED_show(255, 0, 0, 250);
                    break;;
                }
            }
            case 3:
                // Backlight turn on/off
                backLight++;
                if (backLight > 2)
                    backLight = 0;
                if (backLight != 1)
                    IIC_WriteByte(RTC_ADDRESS, ADDR_BACKLIGHT, backLight);
                break;
            case 4: {
                int8_t logbook_event = 1;
                do {
                    strcpy_P(bigbuf, PSTR("Журнал прыжков\n Выход\n Просмотр\n Повтор прыжка\n Очистить журнал\n"));
                    logbook_event = myMenu(bigbuf, logbook_event);
                    switch (logbook_event) {
                        case 1:
                        case -1:
                            // Exit submenu
                            break;
                        case 2: {
                            // view logbook
                            if (current_jump.total_jump_time > 0 && current_jump.total_jump_time < 2047) {
                                int average_freefall_speed_ms = ((current_jump.exit_altitude - current_jump.deploy_altitude) * 2) / current_jump.deploy_time;
                                int average_freefall_speed_kmh = (int)(3.6f * average_freefall_speed_ms);
                                int max_freefall_speed_kmh = (int)(3.6f * current_jump.max_freefall_speed_ms);

                                sprintf_P(bigbuf, PSTR("%d/%d\n%02d.%02d.%04d %02d:%02d\nO:% 4dм M:% 4dм\nР:% 4dм В:% 4dc\nСс: %dм/с (%dкм/ч)\nМс: %dм/с (%dкм/ч)\n"),
                                    1, 1,
                                    current_jump.exit_timestamp.day, current_jump.exit_timestamp.month, current_jump.exit_timestamp.year, current_jump.exit_timestamp.hour, current_jump.exit_timestamp.minute,
                                    current_jump.exit_altitude * 2, current_jump.deploy_altitude * 2,
                                    (current_jump.deploy_altitude - current_jump.canopy_altitude) * 2, current_jump.deploy_time,
                                    average_freefall_speed_ms, average_freefall_speed_kmh,
                                    current_jump.max_freefall_speed_ms, max_freefall_speed_kmh
                                    );
                                // TODO: Navigate through logbook
                                myMenu(bigbuf, 0);
                            }
                            break;
                        }
                        case 4: {
                            // Erase logbook
                            strcpy_P(smallbuf, PSTR(" Нет!!! \n Да. "));
                            strcpy_P(middlebuf, PSTR("Очистить журнал\nпрыжков?"));
                            bigbuf[0] = 0;
                            if (true /*u8g2.userInterfaceMessage(middlebuf, bigbuf, bigbuf, smallbuf) == 2*/) {
                                // Really clear logbook
                                total_jumps = 0;
                                EEPROM.put(EEPROM_JUMP_COUNTER, total_jumps);
                                strcpy_P(smallbuf, PSTR(" Понятно "));
                                strcpy_P(middlebuf, PSTR("Журнал прыжков\nочищен"));
                                //u8g2.userInterfaceMessage(middlebuf, bigbuf, bigbuf, smallbuf);
                                logbook_event = 1; // exit menu
                            }
                            break;
                        }
                    } // switch (logbook_event)
                } while (logbook_event != 1);
                // Logbook
                break;
            }
            case 6: {
                // "Settings" submenu
                int8_t eventSettings = 1;
                byte newAutoPowerOff = IIC_ReadByte(RTC_ADDRESS, ADDR_AUTO_POWEROFF);
                do {
                    char* power_mode_string = powerMode == MODE_DUMB ? "выкл" : "вкл";
                    sprintf_P(bigbuf, PSTR("Настройки\n Выход\n Время\n Дата\n Будильник\n Авторежим: %s\n Автовыкл: %sч\n Версия ПО\n Дамп памяти\n"), power_mode_string, HeartbeatStr(newAutoPowerOff));
                    eventSettings = myMenu(bigbuf, eventSettings);
                    switch (eventSettings) {
                        case 2: {
                            // Time
                            rtc.readTime();
                            byte hour = rtc.hour;
                            byte minute = rtc.minute;
                            if (SetTime(hour, minute)) {
                                rtc.hour = hour;
                                rtc.minute = minute;
                                rtc.set_time();
                            }
                            break;
                        }
                        case 3:
                            break;
                        case 5:
                            // Auto power mode
                            powerMode = powerMode == MODE_DUMB ? MODE_ON_EARTH : MODE_DUMB;
                            break;
                        case 6: {
                            // Auto poweroff
                            newAutoPowerOff = (++newAutoPowerOff) & 3;
                            IIC_WriteByte(RTC_ADDRESS, ADDR_AUTO_POWEROFF, newAutoPowerOff);
                            heartbeat = ByteToHeartbeat(newAutoPowerOff);
                            break;
                        }
                        case 7: {
                            // About
                            showVersion();
                            while(BTN2_RELEASED); // wait for keypress
                            while(BTN2_PRESSED);
                            break;
                        }
                        case 8: {
                            // TODO.
                            Serial.print(F("EEPROM"));
                            for (int i = 0; i < 1024; i++) {
                                if ((i & 15) == 0) {
                                    sprintf_P(smallbuf, PSTR("\n%03x: "), i);
                                    Serial.print(smallbuf);
                                }
                                byte b = EEPROM.read(i);
                                sprintf_P(smallbuf, PSTR("%2x "), b);
                                Serial.print(smallbuf);
                            }
                            Serial.println(F("\n\nRTC NVRAM"));
                            for (int i = 16; i < 256; i++) {
                                if ((i & 15) == 0) {
                                    sprintf_P(smallbuf, PSTR("\n%03x: "), i);
                                    Serial.print(smallbuf);
                                }
                                byte b = IIC_ReadByte(RTC_ADDRESS, i);
                                sprintf_P(smallbuf, PSTR("%2x "), b);
                                Serial.print(smallbuf);
                            }

                            break;
                        }
                    }
                } while (eventSettings != 1);
                break;
            }
            case 7:
                // Power off
                PowerOff();
                break;
            default:
            break;
        }
    } while (event != 1 && event != -1);
}

bool SetTime(byte &hour, byte &minute) {
    byte pos = 0;
    for(;;) {
        u8g2.firstPage();
        pos &= 3;
        do {
            u8g2.setCursor(0, FONT_HEIGHT);
            u8g2.print(F("Текущее время"));
            u8g2.drawHLine(0, FONT_HEIGHT + 1, DISPLAY_WIDTH-1);
            sprintf_P(middlebuf, PSTR("%c%02d:%02d%c"),
                pos == 0 ? '}' : ' ',
                hour,
                minute,
                pos == 1 ? '{' : ' '
            );
            u8g2.setCursor(0, 20);
            u8g2.print(middlebuf);
            sprintf_P(middlebuf, PSTR("%cОтмена %cОК"),
                pos == 2 ? '}' : ' ',
                pos == 3 ? '}' : ' '
            );
            u8g2.setCursor(0, 35);
            u8g2.print(middlebuf);
        } while(u8g2.nextPage());
        byte keyEvent = getKeypress();
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

bool speed_scaler = true;
byte timeWhileBtn1Pressed = 0;

void loop() {
    // If we cannot use interrupts to wake from low-power modes,
    // we need to know a duration of a loop cycle, because we need it to be
    // *exactly* 500ms to achieve a precise speed calculation
    uint32_t this_millis = millis();
    // Read sensors
    current_altitude = myPressure.readAltitude();
    rtc.readTime();
    previousPowerMode = powerMode;
    bool btn1Pressed = BTN1_PRESSED;

    // TODO. Opening menu when jump snapshot has not been saved leads to a broken snapshot.
    // Either disable menu in automatic modes or erase snapshot in such cases.
    // On 32u4 and Mega it is possible to separate buffers while on 328p it is not.
    if (btn1Pressed) {
        // Try to enter menu
        if (timeWhileBtn1Pressed < 16)
            timeWhileBtn1Pressed++;
    } else {
        if (timeWhileBtn1Pressed == 8 || timeWhileBtn1Pressed == 9) {
            userMenu();
            previous_altitude = CLEAR_PREVIOUS_ALTITUDE;
        }
        timeWhileBtn1Pressed = 0;
    }
    
    current_altitude -= ground_altitude; // ground_altitude can be changed via menu

    if ((bstep & 31) == 0 || powerMode == MODE_PREFILL) {
        // Check and refresh battery meter
        batt = analogRead(A0);
        rel_voltage = (int8_t)round((batt-187)*3.23f);
        if (rel_voltage < 0)
            rel_voltage = 0;
        if (rel_voltage > 100)
            rel_voltage = 100;
    }
    
    // Jitter compensation
    int altDiff = current_altitude - last_shown_altitude;
    if (altDiff > 2 || altDiff < -2 || averageSpeed8 > 1 || averageSpeed8 < -1)
        last_shown_altitude = current_altitude;

    processAltitudeChange(speed_scaler);
    bool powerModeChanged =  (powerMode != previousPowerMode);
    if (powerModeChanged)
        bstepInCurrentMode = 0;
    else if (previous_altitude != CLEAR_PREVIOUS_ALTITUDE)
        bstepInCurrentMode++;

    ShowLEDs(powerModeChanged, timeWhileBtn1Pressed);

    u8g2.firstPage();
    do {
        u8g2.setFont(font_status_line);
        u8g2.setCursor(0,6);
        sprintf_P(middlebuf, date_time_template, rtc.day, rtc.month, rtc.year, rtc.hour, bstepInCurrentMode & 1 ? ' ' : ':', rtc.minute);
        u8g2.print(middlebuf);
        u8g2.setCursor(DISPLAY_WIDTH - 16, 6);
        sprintf_P(smallbuf, PSTR("%3d%%"), rel_voltage);
        u8g2.print(smallbuf);

        u8g2.drawHLine(0, 7, DISPLAY_WIDTH-1);

        u8g2.setFont(font_altitude);
        sprintf_P(smallbuf, PSTR("%4d"), last_shown_altitude);
        u8g2.setCursor(0,38);
        u8g2.print(smallbuf);

        u8g2.drawHLine(0, DISPLAY_HEIGHT-8, DISPLAY_WIDTH-1);

        sprintf_P(middlebuf, PSTR("&%1d' % 3d % 3d % 3d"), powerMode, currentVspeed, averageSpeed8, averageSpeed32);
        u8g2.setFont(font_status_line);
        u8g2.setCursor(0, DISPLAY_HEIGHT - 1);
        u8g2.print(middlebuf);
        // Show heartbeat
        byte hbHrs = heartbeat / 7200;
        byte hbMin = ((heartbeat /2) % 3600) / 60;
        sprintf_P(smallbuf, PSTR("%02d:%02d"), hbHrs, hbMin);
        u8g2.setCursor(DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 1);
        u8g2.print(smallbuf);
    } while ( u8g2.nextPage() );

    speed_scaler = (powerMode != MODE_ON_EARTH || bstepInCurrentMode < 50 || btn1Pressed); // true if period 500ms, false for 4s sleep time
    
    if (btn1Pressed) {
        // If BTN1_PRESSED, we cannot use interrupt to wake from sleep/delay, as BTN1 utilizes the same interrupt pin.
        // In that case we will use measured delay instead.
        int delayMs = 500 - (millis() - this_millis);
        if (delayMs > 0)
            delay(delayMs);
    } else {
        // Can use interrupt to wake to achieve maximum time measure precision
        if (speed_scaler)
            rtc.enableSeedInterrupt();

        INTACK = false;
        attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
        if (IsPWMActive()) {
            // When PWM led control is active, we cannot go into power down mode -
            // it will stop timers and break PWM control.
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
    }
    bstep++;
    if (powerMode == MODE_ON_EARTH || powerMode == MODE_DUMB) {
        // Check auto-poweroff. Prevent it in jump modes.
        heartbeat -= speed_scaler ? 1 : 8;
        if (heartbeat <= 0)
            PowerOff();
    }
}
