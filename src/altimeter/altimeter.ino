#include <EEPROM.h>
#include <LowPower.h>
#include <SPI.h>
#include <Wire.h>
#include "osaMPL3115A2.h"
#include "hwconfig.h"
#include "led.h"
#include "keyboard.h"
#include "display.h"

MPL3115A2 myPressure;
PCF8583 rtc;
void(* resetFunc) (void) = 0;

const uint8_t font_status_line[155] U8X8_FONT_SECTION("font_status_line") = 
  "\22\0\2\2\3\3\2\3\4\4\5\0\0\5\377\5\377\0\0\0\0\0~ \4\200d#\10\254\344T"
  "\34\217\0%\10\253d\62j\243\0*\7\253dR\265\32-\5\213f\6.\5\311d\2/\7\253d"
  "\253\62\2\60\7\253\344*\253\2\61\7\253\344\222\254\6\62\7\253\344\232)\15\63\10\253df\312`\1"
  "\64\10\253d\222\32\261\0\65\10\253dF\324`\1\66\7\253\344\246j\1\67\10\253df*#\0\70"
  "\7\253\344Vk\1\71\7\253\344Zr\1:\6\341db\0\0\0\0\4\377\377\0";

char buf8[8];
char buf16[16];
char buf32[32];
byte step = 0;
byte powerMode = 0;

void DoDrawStr(byte x, byte y, char *buf) {
  u8g2.firstPage();
  do {
    u8g2.drawStr(x, y, buf);
  } while(u8g2.nextPage());
}

void ShowText(const byte x, const byte y, const char* text, bool grow = true)
{
  byte maxlen = strlen_P(text);
  if (!maxlen)
    return;
  char *buf = new char[maxlen+1];
  for (byte i = (grow ? 1 : maxlen); i <= maxlen; i++) {
    strncpy_P(buf, text, i);
    buf[i] = 0;
    DoDrawStr(x, y, buf);
    if (grow)
      delay(400);
  }
  delete[] buf;
}

void SleepForewer() {
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
  pinMode(30, INPUT);
  // Enable interrupt pin to wake me
  pinMode(PIN_INTERRUPT, INPUT_PULLUP);

  do {
    attachInterrupt(digitalPinToInterrupt(7), wake, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(digitalPinToInterrupt(7));
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
}

void setup() {
  pinMode(PIN_HWPWR, OUTPUT);
  Serial.begin(9600); // Console

  LED_init();

  pinMode(PIN_SOUND, OUTPUT);
  digitalWrite(PIN_SOUND, 0);

  // Turn ON hardware
  digitalWrite(PIN_HWPWR, 1);

  Wire.begin();
  u8g2.begin();
  //u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
  pinMode(PIN_LIGHT, OUTPUT);
  u8g2.setFont(u8g2_font_helvB10_tr);
  DISPLAY_LIGHT_ON;
  ShowText(0, 24, PSTR("Ni Hao!"));
  delay(3000);
  for (byte i = 0; i < 5; ++i) {
    LED_show(0, 80, 0, 100);
    delay(250);
  }
  DISPLAY_LIGHT_OFF;
 
  //Configure the sensor
  myPressure.begin(); // Get sensor online
  myPressure.setOversampleRate(5); // Set Oversample то 32
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags 
  myPressure.setModeAltimeter(); // Measure altitude above sea level in meters
  
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_BTN3, INPUT_PULLUP);
}

void loop() {
/*
  rtc.day = 3;
  rtc.month = 10;
  rtc.year = 2019;
  rtc.hour = 11;
  rtc.minute = 33;
  rtc.set_time();
*/
  int zeroAltitude = 0;
  EEPROM.get(0, zeroAltitude);
  
  int lastShownAltitude = 0;
  byte backLight = 0;

/*
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

  int batt = 0;
  
  do {
    int altitude = myPressure.readAltitude();
    if (BTN2_PRESSED) {
      zeroAltitude = altitude;
      EEPROM.put(0, zeroAltitude);
      lastShownAltitude = 0;
      LED_show(0, 80, 0, 300);
      while (BTN2_PRESSED); // wait for button release
    }
    altitude -= zeroAltitude;

    if ((step & 31) == 0) {
      batt = analogRead(A0);
      lastShownAltitude = 0;
    }
    if (step > 180)
      powerMode = 1;
      
    short rel_voltage = (short)round((batt-188)*3.23f);
    if (rel_voltage < 0) rel_voltage = 0;
    if (rel_voltage > 100) rel_voltage = 100;

    int altDiff = altitude - lastShownAltitude;
    if (altDiff > 1 || altDiff < -1) {
      lastShownAltitude = altitude;
    }
  
    if (BTN1_PRESSED) {
      backLight = !backLight;
      digitalWrite(PIN_LIGHT, !backLight);
      while (BTN1_PRESSED); // wait for button release
    }

    if (BTN3_PRESSED) {
      LED_show(0, 80, 0, 300);
      byte i = 0;
      while (BTN3_PRESSED){
        // wait for button release
        delay(15);
        ++i;
        if (! (i & 63))
          LED_show(255, 0, 0, 150);
        if (i > 192) {
          SleepForewer();
        }
      }
      powerMode = !powerMode;
      step = 0;
    }

    rtc.get_time();

    u8g2.firstPage();
    do {
      u8g2.setFont(font_status_line);
      u8g2.setCursor(0,6);
      sprintf(buf32, "%02d.%02d.%04d %02d%c%02d",
        rtc.day, rtc.month, rtc.year, rtc.hour, rtc.second & 1 ? ' ' : ':', rtc.minute);
      u8g2.print(buf32);
      u8g2.setCursor(u8g2.getDisplayWidth() - 16, 6);
      sprintf(buf8, "%3d%c", rel_voltage, powerMode ? '#' : '%');
      u8g2.print(buf8);
      
//      u8g2.setFont(u8g2_font_5x7_t_cyrillic);
      u8g2.setFont(u8g2_font_logisoso30_tn);
      
      sprintf(buf8, "%4d", lastShownAltitude);
      u8g2.setCursor(0,40);
      u8g2.print(buf8);

      u8g2.setFont(font_status_line);
      u8g2.setCursor(0,47);
      u8g2.print(batt);

    } while ( u8g2.nextPage() );
    
    //delay(500);
    //attachInterrupt(WAKE_INTERRUPT, wakeUp, LOW);
    
    if (powerMode)
      LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
    else
      delay(500);
      
    step++;
  } while(1);
}
