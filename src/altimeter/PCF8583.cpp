#include <Arduino.h>
#include <Wire.h>
#include "PCF8583.h"

namespace {
  bool IsLeapYear(int year) {
    return !(year % 400) || ((year % 100) && !(year % 4));
  }
}

PCF8583::PCF8583() {
    status_register = 0;
    alarm_register = 0;
}

void PCF8583::begin() {
    Wire.begin();
}

// initialization 
void PCF8583::init() {
    Wire.beginTransmission(0x50);
    Wire.write(0x00);
    Wire.write(status_register);   // Set alarm on int\ will turn to vcc
    Wire.endTransmission();
}

void PCF8583::get_time() {
  Wire.beginTransmission(0x50);
  Wire.write(0x01);
  Wire.endTransmission();
  Wire.requestFrom(0x50, 6);

  ssecond = bcd_to_short(Wire.read());
  second = bcd_to_short(Wire.read());
  minute = bcd_to_short(Wire.read());
  hour   = bcd_to_short(Wire.read());
  byte incoming = Wire.read(); // year/date counter
  day    = bcd_to_short(incoming & 0x3f);
  year   = (int)((incoming >> 6) & 0x03);      // it will only hold 4 years...
  incoming = Wire.read();
  month  = bcd_to_short(incoming & 0x1f);

  //  but that's not all - we need to find out what the base year is
  //  so we can add the 2 bits we got above and find the real year
  Wire.beginTransmission(0x50);
  Wire.write(0x10);
  Wire.endTransmission();
  Wire.requestFrom(0x50, 2);
  year_base = Wire.read();
  year_base = year_base << 8;
  year_base = year_base | Wire.read();
  year = year + year_base;
  init();
}

void PCF8583::set_time() {
  if (!IsLeapYear(year) && 2 == month && 29 == day) {
    month = 3;
    day = 1;
  }
  // Attempt to find the previous leap year
  year_base = year - year % 4;
  if (!IsLeapYear(year_base)) {
    // Not a leap year (new century), make sure the calendar won't use a 29 days February.
    year_base = year - 1;
  }

  Wire.beginTransmission(0x50);
  Wire.write(0x00);
  Wire.write(0xC0);   // stop counting, don't mask
  Wire.endTransmission();

  Wire.beginTransmission(0x50);
  Wire.write(0x02);
  Wire.write(int_to_bcd(second));
  Wire.write(int_to_bcd(minute));
  Wire.write(int_to_bcd(hour));
  Wire.write(((byte)(year - year_base) << 6) | int_to_bcd(day));
  Wire.write((int_to_bcd(month) & 0x1f));
  Wire.endTransmission();

  Wire.beginTransmission(0x50);
  Wire.write(0x10);
  Wire.write(year_base >> 8);
  Wire.write(year_base & 0x00ff);
  Wire.endTransmission();

  init(); // re set the control/status register to 0x04
}

void PCF8583::enableSeedInterrupt() {
  // Write timer alarm register, 0x0f
  Wire.beginTransmission(0x50);
  Wire.write(0x0f);
  Wire.write(0x50);
  Wire.endTransmission();
  alarm_register = (alarm_register & 0x30) | 0xc9; // alarm interrupt enable, alarm timer enable, timer is 1/100s
  writeAlarmControlRegister();
  status_register |= 4;
  init();
}

void PCF8583::disableSeedInterrupt() {
  init();
  alarm_register = (alarm_register & 0x30) | 0x01;
  writeAlarmControlRegister();
}

bool PCF8583::seedInterruptOccur() {
  return !!(readStatusRegister() & 3);
}

byte PCF8583::readStatusRegister() {
    Wire.beginTransmission(0x50);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.requestFrom(0x50, 1);
    return Wire.read();
}

void PCF8583::writeAlarmControlRegister() {
    Wire.beginTransmission(0x50);
    Wire.write(0x08);
    Wire.write(alarm_register);
    Wire.endTransmission();
}

//Get the alarm at 0x09 adress
void PCF8583::get_alarm() {
  Wire.beginTransmission(0x50);
  Wire.write(0x0A); // Set the register pointer to (0x0A) 
  Wire.endTransmission();

  Wire.requestFrom(0x50, 4); // Read 4 values 

  alarm_second = bcd_to_short(Wire.read());
  alarm_minute = bcd_to_short(Wire.read());
  alarm_hour   = bcd_to_short(Wire.read());

  Wire.beginTransmission(0x50);
  Wire.write(0x0E); 
  Wire.endTransmission();

  Wire.requestFrom(0x50, 1); // Read weekday value 
  alarm_day = bcd_to_short(Wire.read());
}

//Set a daily alarm
void PCF8583::set_daily_alarm() {
  alarm_register |= 0x90; // Daily alarm set
  writeAlarmControlRegister();

  Wire.beginTransmission(0x50);
  Wire.write(0x0a); // Set the register pointer to (0x0a)
  Wire.write(int_to_bcd(alarm_second));
  Wire.write(int_to_bcd(alarm_minute));
  Wire.write(int_to_bcd(alarm_hour));
  Wire.write(0x00); // Set 00 at day 
  Wire.endTransmission();
}

void PCF8583::save_int(const byte &address, const int &data) {
    Wire.beginTransmission(0x50);
    Wire.write(0x12+address);
    Wire.write(data);
    Wire.write(data >> 8);
    Wire.endTransmission();
}

int PCF8583::load_int(const byte &address) {
    Wire.beginTransmission(0x50);
    Wire.write(0x12+address);
    Wire.endTransmission();
    Wire.requestFrom(0x50, 2);
    return (Wire.read() | (Wire.read() << 8));
}


short PCF8583::bcd_to_short(byte bcd){
    return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

byte PCF8583::int_to_bcd(short in){
    return ((in / 10) << 4) + (in % 10);
}
