#include <Arduino.h>
#include <Wire.h>
#include "RTC.h"
#include "common.h"

Rtc::Rtc() {
}

// initialization 
void Rtc::init(bool reset_registers) {
#if RTC==RTC_PCF8583
    if (reset_registers) {
        status_register = 0;
        alarm_register = 0;
    }
    IIC_WriteByte(RTC_ADDRESS, 0, status_register);
    last_stored_year = IIC_ReadInt(RTC_ADDRESS, ADDR_LAST_STORED_YEAR);
#elif RTC==RTC_PCF8563
    if (reset_registers) {
        status_register_1 = 0;
        status_register_2 = 0;
    }
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x00);
    Wire.write(status_register_1);
    Wire.write(status_register_2);
    Wire.endTransmission();
#endif
}

void Rtc::readTime() {
#if RTC==RTC_PCF8583
    beginTransmission(RTC_ADDRESS);
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
#elif RTC==RTC_PCF8563
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x03);
    Wire.endTransmission(false);
    Wire.requestFrom(RTC_ADDRESS, 6);
  
    minute = bcd_to_bin(Wire.read() & 0x7F);
    hour = bcd_to_bin(Wire.read() & 0x3F);
    day = bcd_to_bin(Wire.read() & 0x3F);
    Wire.read(); // skip weekday
    month = bcd_to_bin(Wire.read() & 0x1F);
    year = bcd_to_bin(Wire.read()) + EPOCH;
#endif
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
#if RTC==RTC_PCF8583
    IIC_WriteByte(RTC_ADDRESS, 0, 0xc0);
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x05);
    Wire.write(((uint8_t)(year & 3) << 6) | bin_to_bcd(day));
    Wire.write((bin_to_bcd(month) & 0x1f));
    Wire.endTransmission();
    IIC_WriteInt(RTC_ADDRESS, ADDR_YEAR_BASE, year & 0xfffc);
    last_stored_year = year;
    IIC_WriteInt(RTC_ADDRESS, ADDR_LAST_STORED_YEAR, year);

#elif RTC==RTC_PCF8563
    IIC_WriteByte(RTC_ADDRESS, 0, 0x20);
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x05);                       // Start address
    Wire.write(bin_to_bcd(day));             // Day (1-31)
    Wire.write(0);     // Weekday (0-6 = Sunday-Saturday)
    Wire.write(bin_to_bcd(month));   // Month (1-12, century bit (bit 7) = 1)
    Wire.write(bin_to_bcd(year - EPOCH));   // Year (00-99)
    Wire.endTransmission();
#endif

    init(); // re set the control/status register
}

void Rtc::setTime() {
#if RTC==RTC_PCF8583
    IIC_WriteByte(RTC_ADDRESS, 0, 0xc0);
#elif RTC==RTC_PCF8563
    IIC_WriteByte(RTC_ADDRESS, 0, 0x20);
#endif
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x02);
    Wire.write(0);
    Wire.write(bin_to_bcd(minute));
    Wire.write(bin_to_bcd(hour));
    Wire.endTransmission();
    init(); // re set the control/status register
}

#ifdef ALARM_ENABLE
void Rtc::enableAlarmInterrupt() {
#if RTC==RTC_PCF8583
    if (alarm_enable) {
        // Daily alarm set -> alarm int, no timer alarm, daily, no timer int, no timer
        alarm_register = 0x90;
        writeAlarmControlRegister();
        status_register |= 4;
        init();
    }
#elif RTC==RTC_PCF8563
    if (alarm_enable) {
        status_register_2 = 0x2; // Enable alarm interrupt, clear outstanding interrupt
        init();
    }
#endif
}

void Rtc::readAlarm() {
#if RTC==RTC_PCF8583
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x0b); // Set the register pointer to (0x0B) 
    Wire.endTransmission(false);
    Wire.requestFrom(RTC_ADDRESS, 3);

    alarm_minute = bcd_to_bin(Wire.read());
    alarm_hour   = bcd_to_bin(Wire.read());
    alarm_enable = !!(Wire.read());
#elif RTC==RTC_PCF8563
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x09);
    Wire.endTransmission(false);
    Wire.requestFrom(RTC_ADDRESS, 2);
    byte am = Wire.read();
    alarm_enable = (am & 0x80) ? 0 : 1;
    alarm_minute = bcd_to_bin(am & 0x7f);
    alarm_hour = bcd_to_bin(Wire.read()& 0x3f);
#endif
}

//Set a daily alarm
void Rtc::setAlarm() {
#if RTC==RTC_PCF8583
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x09); // Set the register pointer to (0x0a)
    Wire.write(0); // alarm msecs
    Wire.write(0); // alarm seconds
    Wire.write(bin_to_bcd(alarm_minute));
    Wire.write(bin_to_bcd(alarm_hour));
    Wire.write(alarm_enable); // Set 00 at day 
    Wire.endTransmission();
#elif RTC==RTC_PCF8563
    byte mask = alarm_enable ? 0 : 0x80;
    beginTransmission(RTC_ADDRESS);
    Wire.write(0x09);
    Wire.write(bin_to_bcd(alarm_minute) | mask);
    Wire.write(bin_to_bcd(alarm_hour) | mask);
    Wire.endTransmission();
    saveZeroAltitude(loadZeroAltitude());
#endif
}

#endif // ALARM_ENABLE

void Rtc::enableHeartbeat() {
#if RTC==RTC_PCF8583
    alarm_register = 0;
    writeAlarmControlRegister();
    status_register &= 0xfb;
    init();
#elif RTC==RTC_PCF8563
    status_register_2 = 0; // acknowledge alarm and disable its interrupt
    init();
    delay(50);
    IIC_WriteByte(RTC_ADDRESS, 0xd, 0x83); // set clkout to 1hz 50% duty cycle
#endif    
}


void Rtc::disableHeartbeat() {
#if RTC==RTC_PCF8583
    // To disable heartbeat, need to enable alarm but disable its interrupt
    alarm_register = 0;
    status_register |= 4;
    writeAlarmControlRegister();
    init();
#elif RTC==RTC_PCF8563
    IIC_WriteByte(RTC_ADDRESS, 0xd, 0x0);
#endif    
}

#if RTC==RTC_PCF8583
void Rtc::writeAlarmControlRegister() {
    IIC_WriteByte(RTC_ADDRESS, 0x08, alarm_register);
}
#endif

uint8_t Rtc::bcd_to_bin(uint8_t bcd){
    return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

uint8_t Rtc::bin_to_bcd(uint8_t in){
    return ((in / 10) << 4) + (in % 10);
}

int16_t Rtc::loadZeroAltitude() {
#if RTC==RTC_PCF8583
    return IIC_ReadInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE);
#elif RTC==RTC_PCF8563
    int16_t rc = (IIC_ReadByte(RTC_ADDRESS, 0x0b) & 0x7);
    rc = rc << 3;
    rc |= (IIC_ReadByte(RTC_ADDRESS, 0x0c) & 0x7);
    rc = rc << 8;
    rc |= IIC_ReadByte(RTC_ADDRESS, 0x0f);
    return rc - 1000;
#endif
}

void Rtc::saveZeroAltitude(int16_t zeroAltitude) {
#if RTC==RTC_PCF8583
    IIC_WriteInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE, zeroAltitude);
#elif RTC==RTC_PCF8563
    zeroAltitude += 1000;
    IIC_WriteByte(RTC_ADDRESS, 0x0f, zeroAltitude & 0xff); // timer register
    IIC_WriteByte(RTC_ADDRESS, 0x0c, ((zeroAltitude >> 8) & 0x7) | 0x80); // alarm_weekday register
    IIC_WriteByte(RTC_ADDRESS, 0x0b, ((zeroAltitude >> 11) & 0x7) | 0x80); // alarm_day register
#endif
}
