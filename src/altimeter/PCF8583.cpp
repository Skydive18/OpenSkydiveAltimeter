#include <Arduino.h>
#include <Wire.h>
#include "PCF8583.h"
#include "common.h"

PCF8583::PCF8583() {
    status_register = 0;
    alarm_register = 0;
}

// initialization 
void PCF8583::init() {
    IIC_WriteByte(RTC_ADDRESS, 0, status_register);
}

void PCF8583::readTime() {
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

    init();
}

timestamp_t PCF8583::getTimestamp() {
    timestamp_t rc;
    readTime();
    rc.year = year;
    rc.month = month;
    rc.day = day;
    rc.hour = hour;
    rc.minute = minute;
    return rc;
}

void PCF8583::setDate() {
    IIC_WriteByte(RTC_ADDRESS, 0, 0xc0);
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x05);
    Wire.write(((uint8_t)(year & 3) << 6) | bin_to_bcd(day));
    Wire.write((bin_to_bcd(month) & 0x1f));
    Wire.endTransmission();
    IIC_WriteInt(RTC_ADDRESS, ADDR_YEAR_BASE, year & 0xfffc);

    init(); // re set the control/status register to 0x04
}



void PCF8583::setTime() {
    IIC_WriteByte(RTC_ADDRESS, 0, 0xc0);

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x02);
    Wire.write(0);
    Wire.write(bin_to_bcd(minute));
    Wire.write(bin_to_bcd(hour));
    Wire.endTransmission();

    init(); // re set the control/status register to 0x04
}

void PCF8583::enableSeedInterrupt() {
    // Write timer alarm register, 0x0f
    IIC_WriteByte(RTC_ADDRESS, 0x0f, 0x50);
    alarm_register = (alarm_register & 0x30) | 0xc9; // alarm interrupt enable, alarm timer enable, timer is 1/100s
    writeAlarmControlRegister();
    status_register |= 4;
    init();
}

void PCF8583::enableAlarmInterrupt() {
    // Daily alarm set -> alarm int, no timer alarm, daily, no timer int, no timer
    alarm_register = alarm_enable & 1 ? 0x90 : 0x0;
    writeAlarmControlRegister();
    status_register |= 4;
    init();
}

void PCF8583::disableInterrupt() {
    init();
    alarm_register = 0x01;
    writeAlarmControlRegister();
}

void PCF8583::writeAlarmControlRegister() {
    IIC_WriteByte(RTC_ADDRESS, 0x08, alarm_register);
}

void PCF8583::readAlarm() {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x0b); // Set the register pointer to (0x0B) 
    Wire.endTransmission(false);
    Wire.requestFrom(RTC_ADDRESS, 3);

    alarm_minute = bcd_to_bin(Wire.read());
    alarm_hour   = bcd_to_bin(Wire.read());
    alarm_enable = Wire.read();
}

//Set a daily alarm
void PCF8583::setAlarm() {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x09); // Set the register pointer to (0x0a)
    Wire.write(0); // alarm msecs
    Wire.write(0); // alarm seconds
    Wire.write(bin_to_bcd(alarm_minute));
    Wire.write(bin_to_bcd(alarm_hour));
    Wire.write(alarm_enable); // Set 00 at day 
    Wire.endTransmission();
}

uint8_t PCF8583::bcd_to_bin(uint8_t bcd){
    return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

uint8_t PCF8583::bin_to_bcd(uint8_t in){
    return ((in / 10) << 4) + (in % 10);
}
