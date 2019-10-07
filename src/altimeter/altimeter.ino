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

/*
const uint8_t font_altitude[362] U8G2_FONT_SECTION("font_altitude") = 
  "\15\0\4\4\4\5\4\5\6\21\34\0\0\34\371\34\0\0\0\0\0\1M \6\0\20\316\0-\10;"
  "\70\317\360\301\0.\12E\26\256\42$\305\10\0\60\33\317\25\316U\60\325\232\26fV\251J\226\377\317"
  "T\251Zc\242\315\252t\207\0\61\16\310\35\316S\223\7\17\320\324\377\377\1\62!\317\25\316e.\325"
  "\232&eV\351*Y\11a%\213\226,i\262J\223\225\226\254\264d\321\7\37\63\37\317\25\316\360\323"
  "\222EKVZ\262R\223\350\222\226-Z\226N\253 T\302\11#u\246\0\64!\317\25\316E\264\246"
  "DkJ\264\246D\253 U\202\24\21BE\10\25!\364\340\273\242d\351\10\0\65\37\317\25\316\360\7"
  "E\353T\25\233&.\314\224(\245\252,\335\246R\265\306D\233U\351\16\1\66%\317\25\316t.\325"
  "\232\26fJ\224R\225\226\266l\232\270\60S\242\224\252d\71S\245jM\221\66\253\322\35\2\67\33\317"
  "\25\316\360\7\304R\225 U\264\312\242\265,Z\313\242U\26\255e\321\232\1\70(\317\25\316e.\325"
  "\232&eVi\226\225\252\22e\212\264Y\264\246I\231\22\245\64\313JU\211\62&\332\254Jh\10\0"
  "\71\42\317\25\316e.\325\232\26fVi\226W\252J\224\61\341\244\15[\332\246R\265\306D\233U\351"
  "\16\1\0\0\0\4\377\377\0";
*/

char buf8[8];
char buf32[32];
byte bstep = 0;
byte powerMode = 0;
int altitude;
int zeroAltitude;
int lastShownAltitude = 0;
byte backLight = 0;

struct {
  float battGranulationF;
  unsigned int battGranulationD;
  unsigned short jumpCount; // 0-100
  unsigned short nextJumpPtr; // Pointer to next stored jump if count == 100
} settings;

typedef struct {
  // Timestamp
  uint32_t month  : 4;   // 1..12 [4 bits]
  uint32_t date   : 5;   // 1..31 [5 bits]
  uint32_t hour   : 5;   // 00..23 [5 bits]
  uint32_t minute : 6;   // 00..59 [6 bits]
  uint32_t second : 6;   // 00..59 [6 bits]  
  // Jump data
  uint32_t exitAltitude : 11; // Exit altitude / 4, meters (0..8191)
  uint32_t deployAltitude : 11; // Deploy altitude / 4, meters (0..8191)
  uint32_t freefallTime : 10; // Freefall time, seconds (0..512)
  unsigned short maxFreefallSpeedMS; // Max freefall speed, m/s
} JUMP; // structure size: 9 bytes

typedef struct {
  byte blue_th;
  byte green_th;
  byte yellow_th;
  byte red_th;
} LED_PROFILE;

#define LED_PROFILES_START 100
#define JOURNAL_START 124


void DoDrawStr(byte x, byte y, char *buf) {
  u8g2.firstPage();
  do {
    u8g2.drawStr(x, y, buf);
  } while(u8g2.nextPage());
}

void ShowText(const byte x, const byte y, const char* text, bool grow = true)
{
  byte maxlen = strlen_P(text);
  for (byte i = (grow ? 1 : maxlen); i <= maxlen; i++) {
    strncpy_P(buf32, text, i);
    buf32[i] = 0;
    DoDrawStr(x, y, buf32);
    if (grow)
      delay(400);
  }
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

void wake() {

}

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

  LED_init();

  pinMode(PIN_SOUND, OUTPUT);
  digitalWrite(PIN_SOUND, 0);

  // Turn ON hardware
  digitalWrite(PIN_HWPWR, 1);
  rtc.begin();

  u8g2.begin(PIN_BTN2, PIN_BTN3, PIN_BTN1);
  u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
  pinMode(PIN_LIGHT, OUTPUT);
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
  
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_BTN3, INPUT_PULLUP);

  pinMode(PIN_INTERRUPT, INPUT_PULLUP);
}

void userMenu() {
  u8g2.setFont(u8g2_font_5x8_t_cyrillic);
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
      SleepForewer();
    default:
      break;
  }
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
  zeroAltitude = rtc.load_int(0);
  //EEPROM.get(0, zeroAltitude);
  
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
    altitude = myPressure.readAltitude();

    if (BTN2_PRESSED) {
      LED_show(0, 80, 0, 200);
      while (BTN2_PRESSED); // wait for button release
      userMenu();
    }
    altitude -= zeroAltitude;

    if ((bstep & 31) == 0) {
      batt = analogRead(A0);
      lastShownAltitude = 0;
    }
    if (bstep > 180)
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

/*
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
      bstep = 0;
    }
*/

    rtc.get_time();

    u8g2.firstPage();
    do {
      u8g2.setFont(font_status_line);
      u8g2.setCursor(0,6);
      sprintf(buf32, "%02d.%02d.%04d %02d%c%02d",
        rtc.day, rtc.month, rtc.year, rtc.hour, bstep & 1 ? ' ' : ':', rtc.minute);
      u8g2.print(buf32);
      u8g2.setCursor(DISPLAY_WIDTH - 16, 6);
      sprintf(buf8, "%3d%%", rel_voltage);
      u8g2.print(buf8);
      u8g2.drawHLine(0, 7, DISPLAY_WIDTH-1);
      
//      u8g2.setFont(u8g2_font_5x7_t_cyrillic);
//      u8g2.setFont(u8g2_font_logisoso30_tn);
      u8g2.setFont(u8g2_font_logisoso28_tn);
      
      sprintf(buf8, "%4d", lastShownAltitude);
      u8g2.setCursor(0,38);
      u8g2.print(buf8);
      u8g2.drawHLine(0, DISPLAY_HEIGHT-8, DISPLAY_WIDTH-1);

//      u8g2.setFont(font_status_line);
//      u8g2.setCursor(0,47);
//      u8g2.print(rtc.ssecond);

    } while ( u8g2.nextPage() );
    
    //delay(500);
    //attachInterrupt(WAKE_INTERRUPT, wakeUp, LOW);
    
    if (!powerMode) {
      rtc.enableSeedInterrupt();
    }
    attachInterrupt(digitalPinToInterrupt(7), wake, LOW);
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    detachInterrupt(digitalPinToInterrupt(7));
    rtc.disableSeedInterrupt();
      
    bstep++;
  } while(1);
}
