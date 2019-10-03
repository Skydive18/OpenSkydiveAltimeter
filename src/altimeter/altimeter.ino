#include <EEPROM.h>
#include <LowPower.h>
#include <SPI.h>
#include <Wire.h>
#include "SparkFunMPL3115A2.h"

#include "hwconfig.h"

MPL3115A2 myPressure;
PCF8583 rtc;

const uint8_t bdf_font[42] U8X8_FONT_SECTION("font_status_line") = 
  "\1\3\4\2\4\4\1\1\5\10\7\0\0\5\377\5\377\0\0\0\0\0\15\0\13\210\343G&\322\211t"
  "\42\5\0\0\0\4\377\377\0";

char buf8[8];
char buf16[16];
char buf32[32];
int step = 0;
int powerMode = 0;

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
  u8g2.setFont(u8g2_font_helvB10_tr);
  char *sayonara = PSTR("Sayonara )");
  ShowText(0, 24, sayonara);
  delay(3000);
  for (byte i = 0; i < 5; ++i) {
    DoDrawStr(0, 24, "");
    delay(200);
    ShowText(0, 24, sayonara, false);
    delay(200);
  }
  
  digitalWrite(PIN_HWPWR, 0);
  digitalWrite(PIN_R, 0);
  digitalWrite(PIN_G, 0);
  digitalWrite(PIN_B, 0);
  digitalWrite(PIN_LIGHT, 1);
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

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void wake ()
{
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // Console

  // Turn ON hardware
  pinMode(PIN_HWPWR, OUTPUT);
  digitalWrite(PIN_HWPWR, 1);

  Wire.begin();
  u8g2.begin();
  myPressure.begin(); // Get sensor online

  //Configure the sensor

  myPressure.setOversampleRate(5); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags 
  myPressure.setModeAltimeter(); // Measure altitude above sea level in meters
  
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  pinMode(PIN_LIGHT, OUTPUT);
  digitalWrite(PIN_LIGHT, 1);

  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_BTN3, INPUT_PULLUP);
}

//void testLED()
//{
//  for (byte i = 7; i < 8; i--)
//  {
//    digitalWrite(PIN_R, i & 1);
//    digitalWrite(PIN_G, (i >> 1) & 1);
//    digitalWrite(PIN_B, (i >> 2) & 1);
//    delay(2000);
//  }
//}

void loop() {
/*
  rtc.day = 3;
  rtc.month = 10;
  rtc.year = 2019;
  rtc.hour = 11;
  rtc.minute = 33;
  rtc.set_time();
*/  
  float zeroAltitude = 0.0;
  EEPROM.get(0, zeroAltitude);
  
  float lastShownAltitude = 0.0f;
  byte backLight = 0;

  pinMode(PIN_SOUND, OUTPUT);
  tone(PIN_SOUND, 440);
  delay(250);
  noTone(PIN_SOUND);

//  delay(3000);
//  Serial.println("OpenAltimeter boolstrap loader v0.1");  // Print "Hello World" to the Serial Monitor
//  delay(2000);

  // LED test
//  Serial.println("LED test");  // Print "Hello World" to the Serial Monitor
//  testLED();

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

  //u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function

  float battVoltage;

  u8g2.setFont(u8g2_font_helvB10_tr);
  ShowText(0, 24, PSTR("Ni Hao!"));
  delay(3000);

  int batt = 0;
  
  do {
    float altitude = myPressure.readAltitude();
    if (! digitalRead(PIN_BTN2)) {
      zeroAltitude = altitude;
      EEPROM.put(0, zeroAltitude);
      lastShownAltitude = 0;
      while (!digitalRead(PIN_BTN2)); // wait for button release
    }
    altitude -= zeroAltitude;

    if (abs(altitude - lastShownAltitude) >= 2) {
      lastShownAltitude = altitude;
    }
  
    int temperature = (int)myPressure.readTemp();

    if (!digitalRead(PIN_BTN1)) {
      backLight = !backLight;
      digitalWrite(PIN_LIGHT, !backLight);
      while (!digitalRead(PIN_BTN1)); // wait for button release
    }

    if (!digitalRead(PIN_BTN3)) {
      unsigned long millis1 = millis();
      while (!digitalRead(PIN_BTN3)){
        // wait for button release
        delay(50);
        if ((millis() - millis1) > 5000)
        SleepForewer();
      }
      powerMode = !powerMode;
    }

    if ((step & 31) == 0) {
      batt = analogRead(A0);
      battVoltage = (4.20 * (float)batt) / 219;
    }
    short rel_voltage = (short)round(((battVoltage - 3.6) / (4.20 - 3.6)) * 100);
    if (rel_voltage < 0) rel_voltage = 0;
    if (rel_voltage > 100) rel_voltage = 100;

    rtc.get_time();

    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_4x6_tn);
      u8g2.setCursor(0,6);
      sprintf(buf32, "%02d.%02d.%04d %02d%c%02d",
        rtc.day, rtc.month, rtc.year, rtc.hour, rtc.second & 1 ? ' ' : ':', rtc.minute);
      u8g2.print(buf32);
      u8g2.setCursor(u8g2.getDisplayWidth() - 16, 6);
      sprintf(buf8, "%3d%c", rel_voltage, powerMode ? '*' : '-');
      u8g2.print(buf8);
      
//      u8g2.setFont(u8g2_font_5x7_t_cyrillic);
      u8g2.setFont(u8g2_font_logisoso30_tn);
      
      sprintf(buf8, "%4d", (int)round(lastShownAltitude));
      u8g2.setCursor(0,40);
      u8g2.print(buf8);

      dtostrf(battVoltage, 4, 2, buf16);
      sprintf(buf32, "%d %s %d", batt, buf16, temperature);
      u8g2.setFont(u8g2_font_4x6_tn);
      u8g2.setCursor(0,47);
      u8g2.print(buf32);

    } while ( u8g2.nextPage() );
    
    //delay(500);
    //attachInterrupt(WAKE_INTERRUPT, wakeUp, LOW);
    
    if (powerMode)
      LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
    else
      delay(500);
      
    //detachInterrupt(WAKE_INTERRUPT);
    step++;
  } while(1);
}
