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
short vspeed[VSPEED_LENGTH]; // used in mode detector. TODO clean it in setup maybe?
uint32_t bstep = 0; // global heartbeat counter
uint32_t bstepInCurrentMode; //heartbeat counter since mode changed
byte vspeedPtr = 0; // pointer to write to vspeed buffer
int currentVspeed;
short averageSpeed;
short averageSpeed32;
short averageSpeed2;

// ********* Main power move flag
byte powerMode;
byte previousPowerMode;

int altitude; // currently measured altitude, relative to zeroAltitude
int previousAltitude; // Previously measured altitude, relative to zeroAltitude
int zeroAltitude; // Ground altitude, absolute
int lastShownAltitude; // Last shown altitude (jitter corrected)
byte backLight = 0; // Display backlight flag, 0=off, 1=on
int batt; // battery voltage as it goes from sensor
short rel_voltage; // ballery relative voltare, in percent (0-100). 0 relates to 3.6v and 100 relates to fully charged batt

char buf8[48];
char bigbuf[200];


short getAverageVspeed(byte averageLength) {
    byte startPtr = (byte)((vspeedPtr - averageLength)) & (VSPEED_LENGTH - 1);
    int accumulatedVspeed = 0;
    for (byte i = 0; i < averageLength; i++)
        accumulatedVspeed += vspeed[(startPtr + i) & (VSPEED_LENGTH - 1)];
    return (short)(accumulatedVspeed / averageLength);
}

bool processAltitudeChange() {
    if (previousAltitude == CLEAR_PREVIOUS_ALTITUDE) { // unknown delay or first run; just save
        previousAltitude = altitude;
        return false;
    }
    currentVspeed = altitude - previousAltitude; // negative = fall, positive = climb
    previousAltitude = altitude;
    if (powerMode /* != MODE_ON_EARTH*/)
        currentVspeed *= 2; // 500ms granularity
    else
        currentVspeed /= 4; // 4s granularity
    vspeed[vspeedPtr & (VSPEED_LENGTH - 1)] = (short)currentVspeed;
    averageSpeed = (powerMode != MODE_PREFILL) ? getAverageVspeed(8) : 0;
    averageSpeed2 = (powerMode != MODE_PREFILL) ? getAverageVspeed(2) : 0;
    averageSpeed32 = (powerMode != MODE_PREFILL) ? getAverageVspeed(32) : 0;
    vspeedPtr++;

    // Strictly set freefall in automatic modes if vspeed < freefall_th
    if (powerMode != MODE_DUMB && powerMode != MODE_PREFILL && powerMode != MODE_FREEFALL && averageSpeed < -25) {
        powerMode = MODE_FREEFALL;
        return true;
    }
    
    switch (powerMode) {
        case MODE_DUMB:
            // No logic in this mode. Manual change only.
            return false;
        
        case MODE_PREFILL:
            // Change mode to ON_EARTH if vspeed had been prefilled (approx. 16s after enter to this mode)
            if (vspeedPtr > VSPEED_LENGTH)
                powerMode = MODE_ON_EARTH;
            return (powerMode != MODE_PREFILL);
        
        case MODE_ON_EARTH:
            // Altitude granularity is 4s
            if ( (altitude > 250) || (altitude > 30 && averageSpeed > 1) )
                powerMode = MODE_IN_AIRPLANE;
            else
                break;
            return true;

        case MODE_IN_AIRPLANE:
            if (currentVspeed <= EXIT_TH)
                powerMode = MODE_EXIT;
            else if (averageSpeed32 < 1 && altitude < 25)
                powerMode = MODE_ON_EARTH;
            else
                break;
            return true;

        case MODE_EXIT:
            if (currentVspeed <= BEGIN_FREEFALL_TH)
                powerMode = MODE_BEGIN_FREEFALL;
            else if (currentVspeed > EXIT_TH)
                powerMode = MODE_IN_AIRPLANE;
            else
                break;
        return true;

        case MODE_BEGIN_FREEFALL:
            if (currentVspeed <= FREEFALL_TH)
                powerMode = MODE_FREEFALL;
            else if (currentVspeed > BEGIN_FREEFALL_TH)
                powerMode = MODE_EXIT;
            else
                break;
        return true;

        case MODE_FREEFALL:
            if (currentVspeed > PULLOUT_TH)
                powerMode = MODE_PULLOUT;
            else
                break;
        return true;

        case MODE_PULLOUT:
            if (currentVspeed <= PULLOUT_TH)
                powerMode = MODE_FREEFALL;
            else if (currentVspeed > OPENING_TH)
                powerMode = MODE_OPENING;
            else
                break;
        return true;

        case MODE_OPENING:
            if (currentVspeed <= OPENING_TH)
                powerMode = MODE_PULLOUT;
            else if (currentVspeed > UNDER_PARACHUTE_TH)
                powerMode = MODE_UNDER_PARACHUTE;
            else
                break;
        return true;

        case MODE_UNDER_PARACHUTE:
            if (averageSpeed32 < 1 && altitude < 25)
                powerMode = MODE_ON_EARTH;
            else
                break;
        return true;
    }

    return false;
}

byte airplane_300m = 0;
void ShowLEDs(bool powerModeChanged) {
    switch (powerMode) {
        case MODE_IN_AIRPLANE:
            if (altitude >= 300) {
                if (airplane_300m <= 6 && altitude < 900) {
                    LED_show(0, ((++airplane_300m) & 1) ? 255 : 0, 0); // green blinks 3 times at 300m but not above 900m
                } else {
                    if (altitude > 900) {
                        LED_show(0, (bstep & 15) ? 0 : 255, 0); // green blinks once per 8s above 900m
                    } else {
                        LED_show((bstep & 15) ? 0 : 255, (bstep & 15) ? 0 : 160, 0); // yellow blinks once per 8s between 300m and 900m
                    }
                }
            } else {
                LED_show((bstep & 15) ? 0 : 255, 0, 0); // red blinks once per 8s below 300m
            }
            return;
        case MODE_FREEFALL:
            if (altitude < 1000) {
                LED_show(255, 0, 0); // red ligth below 1000m
            } else if (altitude < 1200) {
                LED_show(255, 160, 0); // yellow ligth between 1200 and 1000m
            } else if (altitude < 1550) {
                LED_show(0, 255, 0); // green ligth between 1500 and 1200m
            } else {
                LED_show(0, 0, 255); // blue ligth between 1500 and 1200m
            }
            return;

        case MODE_PREFILL:
            LED_show((bstep & 3) ? 0 : 255, 0, 0); // red blinks once per 2s until prefill finished
            return;
    }

    airplane_300m = 0;
    LED_show(0, 0, 0);
}

void DoDrawStr(byte x, byte y, char *buf) {
    u8g2.firstPage();
    do {
        u8g2.drawStr(x, y, buf);
    } while(u8g2.nextPage());
}

void ShowText(const byte x, const byte y, const char* text, bool grow = true) {
    byte maxlen = strlen_P(text);
    for (byte i = (grow ? 1 : maxlen); i <= maxlen; i++) {
        strncpy_P(bigbuf, text, i);
        bigbuf[i] = 0;
        DoDrawStr(x, y, bigbuf);
        if (grow)
        off_250ms;
    }
}

void SleepForever() {
    DISPLAY_LIGHT_ON;
    u8g2.setFont(u8g2_font_helvB10_tr);
    char *sayonara = PSTR("Sayonara )");
    ShowText(0, 24, sayonara);
    off_4s;
    for (byte i = 0; i < 5; ++i) {
        ShowText(0, 24, sayonara, false);
        off_250ms;
        DoDrawStr(0, 24, "");
        off_250ms;
    }
  
    DISPLAY_LIGHT_OFF;

    rtc.disableSeedInterrupt();
  
    digitalWrite(PIN_HWPWR, 0);
    digitalWrite(PIN_SOUND, 0);
    // turn off i2c
    pinMode(2, INPUT);
    pinMode(3, INPUT);
    // turn off SPI
    pinMode(MOSI, INPUT);
    pinMode(MISO, INPUT);
    pinMode(SCK, INPUT);
    pinMode(SS, INPUT);
#ifdef DC_PIN    
    pinMode(DC_PIN, INPUT);
#endif
    // Enable interrupt pin to wake me
    pinMode(PIN_INTERRUPT, INPUT_PULLUP);

    do {
        attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
        off_forever;
        detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    } while (!checkWakeCondition());
    resetFunc();
}

void wake() {}

byte checkWakeCondition ()
{
  // Determine wake condition
    pinMode(PIN_BTN1, INPUT_PULLUP);
    off_15ms;
    if (BTN1_PRESSED) {
        // Wake by awake button. Should be kept pressed for 3s
        LED_show(0, 0, 80, 400);
        for (byte i = 1; i < 193; ++i) {
        if (BTN1_RELEASED)
            return 0;
        if (! (i & 63))
            LED_show(0, 80, 0, 200);
        off_15ms;
        }
        return 1;
    }

    return 0;
}

void setup() {
    pinMode(PIN_HWPWR, OUTPUT);
    Serial.begin(9600); // Console

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

    LED_init();

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
    myPressure.setOversampleRate(6); // Set Oversample то 64

    // Configure keyboard
    pinMode(PIN_BTN1, INPUT_PULLUP);
    pinMode(PIN_BTN2, INPUT_PULLUP);
    pinMode(PIN_BTN3, INPUT_PULLUP);

    pinMode(PIN_INTERRUPT, INPUT_PULLUP);

    // Read presets
    lastShownAltitude = 0;
    powerMode = previousPowerMode = MODE_PREFILL;
    previousAltitude = CLEAR_PREVIOUS_ALTITUDE;
    vspeedPtr = 0;
    bstepInCurrentMode = 0;
    
    // Show greeting message
    u8g2.setFont(u8g2_font_helvB10_tr);
    DISPLAY_LIGHT_ON;
    ShowText(16, 30, PSTR("Ni Hao!"));
    off_4s;
    for (byte i = 0; i < 5; ++i) {
        LED_show(0, 80, 0, 100);
        off_250ms;
    }
    DISPLAY_LIGHT_OFF;
}

void userMenu() {
    u8g2.setFont(font_menu);

    short event = 1;
    do {
        strcpy_P(bigbuf, PSTR("Выход\nСброс на ноль\nЖурнал прыжков\nСигналы\nНастройки\nВыключение"));
        strcpy_P(buf8, PSTR("Меню"));
        event = u8g2.userInterfaceSelectionList(buf8, event, bigbuf);
        switch (event) {
            case 2: {
                zeroAltitude = altitude;
                IIC_WriteInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE, zeroAltitude);
                lastShownAltitude = 0;
                return;
            }
            case 5: {
                short eventSettings = 1;
                do {
                    strcpy_P(bigbuf, PSTR("Выход\nВремя\nДата\nБудильник\nРежим вручную"));
                    strcpy_P(buf8, PSTR("Настройки"));
                    eventSettings = u8g2.userInterfaceSelectionList(buf8, eventSettings, bigbuf);
                    switch (eventSettings) {
                        case 5: {
                            strcpy_P(bigbuf, PSTR("Не менять\nНа земле\nВ самолёте\nБез обработки"));
                            strcpy_P(buf8, PSTR("Режим вручную"));
                            short eventModeManually = u8g2.userInterfaceSelectionList(buf8, 1, bigbuf);
                            switch (eventModeManually) {
                                case 2: powerMode = MODE_ON_EARTH; return;
                                case 3: powerMode = MODE_IN_AIRPLANE; return;
                                case 4: powerMode = MODE_DUMB; return;
                            }
                            break;
                        }
                    }
                } while (eventSettings != 1);
                break;
            }
            case 6:
                SleepForever();
                break;
            default:
            break;
        }
    } while (event != 1);
}

void loop() {
    // Read sensors
    altitude = myPressure.readAltitude();
    zeroAltitude = IIC_ReadInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE);
    rtc.get_time();
    previousPowerMode = powerMode;

    // Backlight button
    if (BTN1_PRESSED) {
        backLight = !backLight;
        digitalWrite(PIN_LIGHT, !backLight);
        while (BTN1_PRESSED); // wait for button release
        previousAltitude = CLEAR_PREVIOUS_ALTITUDE;
    }
    
    // User menu
    if (BTN2_PRESSED) {
        LED_show(0, 80, 0, 200);
        while (BTN2_PRESSED); // wait for button release
        userMenu();
        previousAltitude = CLEAR_PREVIOUS_ALTITUDE;
    }
    altitude -= zeroAltitude;

    if ((bstep & 31) == 0) {
        batt = analogRead(A0);
        rel_voltage = (short)round((batt-188)*3.23f);
        if (rel_voltage < 0)
            rel_voltage = 0;
        if (rel_voltage > 100)
            rel_voltage = 100;
    }
    
    // Jitter compensation
    int altDiff = altitude - lastShownAltitude;
    if (altDiff > 2 || altDiff < -2 || averageSpeed > 1 || averageSpeed < -1) {
        lastShownAltitude = altitude;
    }

    u8g2.firstPage();
    do {
        u8g2.setFont(font_status_line);
        u8g2.setCursor(0,6);
        sprintf_P(bigbuf, PSTR("%02d.%02d.%04d %02d%c%02d"), rtc.day, rtc.month, rtc.year, rtc.hour, bstep & 1 ? ' ' : ':', rtc.minute);
        u8g2.print(bigbuf);
        u8g2.setCursor(DISPLAY_WIDTH - 16, 6);
        sprintf_P(buf8, PSTR("%3d%%"), rel_voltage);
        u8g2.print(buf8);

        u8g2.drawHLine(0, 7, DISPLAY_WIDTH-1);

        u8g2.setFont(font_altitude);
        sprintf_P(buf8, PSTR("%4d"), lastShownAltitude);
        u8g2.setCursor(0,38);
        u8g2.print(buf8);

        u8g2.drawHLine(0, DISPLAY_HEIGHT-8, DISPLAY_WIDTH-1);

        sprintf_P(buf8, PSTR("-%1d- % 3d % 3d % 3d % 3d"), powerMode, currentVspeed, averageSpeed, averageSpeed2, averageSpeed32);
        u8g2.setFont(font_status_line);
        u8g2.setCursor(0,47);
        u8g2.print(buf8);

    } while ( u8g2.nextPage() );
  
    if (powerMode /*!= ON_EARTH*/) {
        rtc.enableSeedInterrupt();
    }
    attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    rtc.disableSeedInterrupt();

    bool powerModeChanged = processAltitudeChange() || (powerMode != previousPowerMode);
    if (powerModeChanged)
        bstepInCurrentMode = 0;
    else if (previousAltitude != CLEAR_PREVIOUS_ALTITUDE)
        bstepInCurrentMode++;

    ShowLEDs(powerModeChanged);

    bstep++;
}
