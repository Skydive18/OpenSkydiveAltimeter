#include <EEPROM.h>
#include <LowPower.h>
#include <SPI.h>
#include <Wire.h>
#include "osaMPL3115A2.h"
#include "hwconfig.h"
#include "led.h"
#include "keyboard.h"
#include "display.h"
#include "fonts.h"
#include "custom_types.h"

// Altimeter modes
#define ON_EARTH 0
#define IN_AIRPLANE 1
#define FREEFALL 2
#define UNDER_PARACHUTE 3

MPL3115A2 myPressure;
PCF8583 rtc;
void(* resetFunc) (void) = 0;

char buf8[8];
char buf32[32];
short vspeed[32]; // used in mode detector. TODO clean it in setup
uint32_t bstep = 0;
byte powerMode = 0;
int altitude;
int previousAltitude = -30000;
int zeroAltitude;
int lastShownAltitude = 0;
byte backLight = 0;
int batt;
short rel_voltage;

short getAverageVspeed(byte averageLength) {
    byte startPtr = (byte)((bstep - averageLength)) & 31;
    int accumulatedVspeed = 0;
    for (byte i = 0; i < averageLength; i++)
        accumulatedVspeed += vspeed[(startPtr + i) & 31];
    return (short)(accumulatedVspeed / averageLength);
}

short getAverageAcceleration(byte averageLength) {
    return 0;
}

bool processAltitudeChange() {
    if (previousAltitude == -10000) { // first run
        previousAltitude = altitude;
        return false;
    }
    int currentVspeed = altitude - previousAltitude;
    previousAltitude = altitude;
    if (powerMode /* != ON_EARTH*/)
        currentVspeed *= 2; // 500ms granularity
    else
        currentVspeed /= 4; // 4s granularity
    vspeed[bstep & 31] = (short)currentVspeed;

    switch (powerMode) {
        case ON_EARTH: {
            // Altitude granularity is 4s
            short averageSpeed = getAverageVspeed(8);
            if ( (altitude > 250) || (altitude > 30 && bstep > 7 && averageSpeed > 1) )
                powerMode = IN_AIRPLANE;
            else if (averageSpeed < 10)
                powerMode = FREEFALL;
            else
                break;
            return true;
        }
        case IN_AIRPLANE: {
            // Altitude granularity is 500ms
            // Check acceleration to detect freefall begin
            break;
        }
    }

    return false;
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
        strncpy_P(buf32, text, i);
        buf32[i] = 0;
        DoDrawStr(x, y, buf32);
        if (grow)
        delay(400);
    }
}

void SleepForever() {
    DISPLAY_LIGHT_ON;
    u8g2.setFont(u8g2_font_helvB10_tr);
    char *sayonara = PSTR("Sayonara )");
    ShowText(0, 24, sayonara);
    delay(2500);
    for (byte i = 0; i < 5; ++i) {
        ShowText(0, 24, sayonara, false);
        delay(200);
        DoDrawStr(0, 24, "");
        delay(200);
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
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    } while (!checkWakeCondition());
    resetFunc();
}

void wake() {}

byte checkWakeCondition ()
{
  // Determine wake condition
    pinMode(PIN_BTN1, INPUT_PULLUP);
    delay(20);
    if (BTN1_PRESSED) {
        // Wake by awake button. Should be kept pressed for 3s
        LED_show(0, 0, 80, 400);
        for (byte i = 1; i < 193; ++i) {
        if (BTN1_RELEASED)
            return 0;
        if (! (i & 63))
            LED_show(0, 80, 0, 200);
        delay(15);
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
    
    // Show greeting message
    u8g2.setFont(u8g2_font_helvB10_tr);
    DISPLAY_LIGHT_ON;
    ShowText(16, 30, PSTR("Ni Hao!"));
    delay(3000);
    for (byte i = 0; i < 5; ++i) {
        LED_show(0, 80, 0, 100);
        delay(250);
    }
    DISPLAY_LIGHT_OFF;
 
    //Configure the sensor
    myPressure.begin(); // Get sensor online
    myPressure.setOversampleRate(6); // Set Oversample то 64
  
    // Configure keyboard
    pinMode(PIN_BTN1, INPUT_PULLUP);
    pinMode(PIN_BTN2, INPUT_PULLUP);
    pinMode(PIN_BTN3, INPUT_PULLUP);

    pinMode(PIN_INTERRUPT, INPUT_PULLUP);

    // Read presets
    zeroAltitude = rtc.load_int(0);
}

void userMenu() {
    u8g2.setFont(font_menu);
    short event = u8g2.userInterfaceSelectionList("Меню", 1, "Выход\nСброс на ноль\nЖурнал прыжков\nСигналы\nНастройки\nВыключение");
    switch (event) {
        case 2: {
            zeroAltitude = altitude;
            rtc.save_int(0, zeroAltitude);
            lastShownAltitude = 0;
            break;
        }
        case 5: {
            short eventSettings = u8g2.userInterfaceSelectionList("Настройки", 1, "Выход\nВремя\nДата\nБудильник\nТекущая высота\nАвтовыключение");
            break;
        }
        case 6:
            SleepForever();
            break;
        default:
        break;
    }
}

void loop() {
    // Read sensors
    altitude = myPressure.readAltitude();
    rtc.get_time();

    // Backlight button
    if (BTN1_PRESSED) {
        backLight = !backLight;
        digitalWrite(PIN_LIGHT, !backLight);
        while (BTN1_PRESSED); // wait for button release
    }
    
    // User menu
    if (BTN2_PRESSED) {
        LED_show(0, 80, 0, 200);
        while (BTN2_PRESSED); // wait for button release
        userMenu();
    }
    altitude -= zeroAltitude;

    if ((bstep & 31) == 0) {
        batt = analogRead(A0);
        lastShownAltitude = 0;
        rel_voltage = (short)round((batt-188)*3.23f);
        if (rel_voltage < 0)
            rel_voltage = 0;
        if (rel_voltage > 100)
            rel_voltage = 100;
    }
    
    if (bstep > 180)
        powerMode = 1;

    int altDiff = altitude - lastShownAltitude;
    if (altDiff > 1 || altDiff < -1) {
        lastShownAltitude = altitude;
    }

    u8g2.firstPage();
    do {
        u8g2.setFont(font_status_line);
        u8g2.setCursor(0,6);
        sprintf(buf32, "%02d.%02d.%04d %02d%c%02d", rtc.day, rtc.month, rtc.year, rtc.hour, bstep & 1 ? ' ' : ':', rtc.minute);
        u8g2.print(buf32);
        u8g2.setCursor(DISPLAY_WIDTH - 16, 6);
        sprintf(buf8, "%3d%%", rel_voltage);
        u8g2.print(buf8);

        u8g2.drawHLine(0, 7, DISPLAY_WIDTH-1);

        u8g2.setFont(font_altitude);
        sprintf(buf8, "%4d", lastShownAltitude);
        u8g2.setCursor(0,38);
        u8g2.print(buf8);

        u8g2.drawHLine(0, DISPLAY_HEIGHT-8, DISPLAY_WIDTH-1);

//      u8g2.setFont(font_status_line);
//      u8g2.setCursor(0,47);
//      u8g2.print(rtc.ssecond);

    } while ( u8g2.nextPage() );
  
    if (powerMode /*!= ON_EARTH*/) {
        rtc.enableSeedInterrupt();
    }
    attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wake, LOW);
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    rtc.disableSeedInterrupt();

    if (processAltitudeChange()) {
        bstep = 0;
    } else {
        bstep++;
    }
}
