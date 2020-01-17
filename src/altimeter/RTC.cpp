#include <Arduino.h>
#include <Wire.h>
#include "RTC.h"
#include "common.h"

Rtc::Rtc() {
    status_register = 0;
    alarm_register = 0;
}

// initialization 
void Rtc::init() {
    IIC_WriteByte(RTC_ADDRESS, 0, status_register);
    last_stored_year = IIC_ReadInt(RTC_ADDRESS, ADDR_LAST_STORED_YEAR);
}

void Rtc::readTime() {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x03);
    Wire.endTransmission(false);
    Wire.requestFrom(RTC_ADDRESS, 4);

    minute = bcd_to_bin(Wire.read());
    hour   = bcd_to_bin(Wire.read());
    uint8_t incoming = Wire.read(); // year/date counter
    day    = bcd_to_bin(incoming & 0x3f);
    year   = (uint16_t)((incoming >> 6) & 0x03);      // it will only hold 4 years...
    incoming = Wire.read();
    month  = bcd_to_bin(incoming & 0x1f);
    year += IIC_ReadInt(RTC_ADDRESS, ADDR_YEAR_BASE);
    if (year < last_stored_year && (last_stored_year - year) <= 4) { // 4-year counter overlap
        year += 4;
        setDate();
    }
}

timestamp_t Rtc::getTimestamp() {
    timestamp_t rc;
    readTime();
    rc.year = year;
    rc.month = month;
    rc.day = day;
    rc.hour = hour;
    rc.minute = minute;
    return rc;
}

void Rtc::setDate() {
    IIC_WriteByte(RTC_ADDRESS, 0, 0xc0);
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x05);
    Wire.write(((uint8_t)(year & 3) << 6) | bin_to_bcd(day));
    Wire.write((bin_to_bcd(month) & 0x1f));
    Wire.endTransmission();
    IIC_WriteInt(RTC_ADDRESS, ADDR_YEAR_BASE, year & 0xfffc);
    last_stored_year = year;
    IIC_WriteInt(RTC_ADDRESS, ADDR_LAST_STORED_YEAR, year);

    init(); // re set the control/status register to 0x04
}

void Rtc::setTime() {
    IIC_WriteByte(RTC_ADDRESS, 0, 0xc0);

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x02);
    Wire.write(0);
    Wire.write(bin_to_bcd(minute));
    Wire.write(bin_to_bcd(hour));
    Wire.endTransmission();

    init(); // re set the control/status register to 0x04
}

#if defined (__AVR_ATmega32U4__)
void Rtc::enableSeedInterrupt() {
    // Generate normal interrupt using timer match
    // Write timer alarm register, 0x0f
    IIC_WriteByte(RTC_ADDRESS, 0x0f, 0x50);
    alarm_register = (alarm_register & 0x30) | 0xc9; // alarm interrupt enable, alarm timer enable, timer is 1/100s
    writeAlarmControlRegister();
    status_register |= 4;
    init();
}
#endif

#ifdef ALARM_ENABLE
void Rtc::enableAlarmInterrupt() {
    // Daily alarm set -> alarm int, no timer alarm, daily, no timer int, no timer
    alarm_register = alarm_enable & 1 ? 0x90 : 0x0;
    writeAlarmControlRegister();
    status_register |= 4;
    init();
}

void Rtc::readAlarm() {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x0b); // Set the register pointer to (0x0B) 
    Wire.endTransmission(false);
    Wire.requestFrom(RTC_ADDRESS, 3);

    alarm_minute = bcd_to_bin(Wire.read());
    alarm_hour   = bcd_to_bin(Wire.read());
    alarm_enable = Wire.read();
}

//Set a daily alarm
void Rtc::setAlarm() {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x09); // Set the register pointer to (0x0a)
    Wire.write(0); // alarm msecs
    Wire.write(0); // alarm seconds
    Wire.write(bin_to_bcd(alarm_minute));
    Wire.write(bin_to_bcd(alarm_hour));
    Wire.write(alarm_enable); // Set 00 at day 
    Wire.endTransmission();
}

#endif // ALARM_ENABLE

void Rtc::disableInterrupt() {
    init();
    alarm_register = 0x01;
    writeAlarmControlRegister();
}

void Rtc::writeAlarmControlRegister() {
    IIC_WriteByte(RTC_ADDRESS, 0x08, alarm_register);
}

uint8_t Rtc::bcd_to_bin(uint8_t bcd){
    return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

uint8_t Rtc::bin_to_bcd(uint8_t in){
    return ((in / 10) << 4) + (in % 10);
}
