#include <EEPROM.h>

#include <LowPower.h>

#include <SPI.h>
#include <Wire.h>
#include "SparkFunMPL3115A2.h"
#include <U8g2lib.h>
#include <U8x8lib.h>

#define PIN_HWPWR 1
#define PIN_LIGHT 4
#define PIN_R 6
#define PIN_G 9
#define PIN_B 10
#define PIN_BTN1 A3
#define PIN_BTN2 A2
#define PIN_BTN3 A1
#define PIN_BAT_SENSE A0
#define PIN_SOUND 5
#define PIN_INTERRUPT 7
#define WAKE_INTERRUPT 6

//Create an instance of the object
MPL3115A2 myPressure;
U8G2_PCD8544_84X48_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ SS, /* dc=*/ 30, /* reset=*/ U8X8_PIN_NONE);            // Nokia 5110 Display
//U8G2_PCD8544_84X48_1_4W_SW_SPI u8g2(U8G2_R0, SCK, MOSI, /* cs=*/ 17, /* dc=*/ 24, /* reset=*/ 8);            // Nokia 5110 Display
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

void testLED()
{
  for (byte i = 7; i < 8; i--)
  {
    digitalWrite(PIN_R, i & 1);
    digitalWrite(PIN_G, (i >> 1) & 1);
    digitalWrite(PIN_B, (i >> 2) & 1);
    delay(2000);
  }
}

void loop() {
  float zeroAltitude = 0.0;
  EEPROM.get(0, zeroAltitude);
  
  float lastShownAltitude = 0.0f;
  byte backLight = 0;

//  Serial.println("OpenAltimeter boolstrap loader v0.1");  // Print "Hello World" to the Serial Monitor
//  delay(2000);

//  Serial.println("Vibro test");  // Print "Hello World" to the Serial Monitor
//  // vibro test
//  pinMode(1, OUTPUT);
//  digitalWrite(0, 1);
//  delay(1000);
//  digitalWrite(0, 0);

  // LED test
//  Serial.println("LED test");  // Print "Hello World" to the Serial Monitor
//  testLED();

//  Serial.println("Display back light test");  // Print "Hello World" to the Serial Monitor
//  // Display back light test
//  pinMode(4, OUTPUT);
//  digitalWrite(4, 1);
//  delay(5000);
//  digitalWrite(4, 0);

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
  
//    float temperature = myPressure.readTemp();

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
      int batt = analogRead(A0);
      battVoltage = (4.16 * (float)batt) / 217;
    }

    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_4x6_tn);
      u8g2.setCursor(0,6);
      u8g2.print("01.10.2019 00:00");
      u8g2.setCursor(u8g2.getDisplayWidth() - 16, 6);
      short rel_voltage = (short)round(((battVoltage - 3.6) / (4.16 - 3.6)) * 100);
      if (rel_voltage < 0) rel_voltage = 0;
      if (rel_voltage > 100) rel_voltage = 100;
      sprintf(buf8, "%3d%c", rel_voltage, powerMode ? '*' : '-');
      u8g2.print(buf8);
      
//      u8g2.setFont(u8g2_font_5x7_t_cyrillic);
      u8g2.setFont(u8g2_font_logisoso30_tn);
      
      sprintf(buf8, "%4d", (int)round(lastShownAltitude));
      u8g2.setCursor(0,42);
      u8g2.print(buf8);

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
