#ifndef __in_PCF8583_h
#define __in_PCF8583_h

#include <Arduino.h>
#include <Wire.h>
#include "hwconfig.h"
#include "custom_types.h"

// Addressing in RAM, altimeter runtime parameters
#define ADDR_YEAR_BASE 0x10
#define ADDR_LAST_STORED_YEAR 0x14
#define ADDR_ZERO_ALTITUDE 0x12

#define RTC_ADDRESS 0x50
// RTC epoch year
#define EPOCH 2016

class PCF8583 {
public:
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint16_t last_stored_year;

#ifdef ALARM_ENABLE
    uint8_t alarm_minute;
    uint8_t alarm_hour;
    uint8_t alarm_enable;
    void enableAlarmInterrupt();
    void readAlarm();
    void setAlarm();
#endif

#if defined(__AVR_ATmega32U4__)
    void enableSeedInterrupt();
#endif
    void disableInterrupt();
    
    PCF8583();
    void init();
    
    void readTime();
    timestamp_t getTimestamp();
    void setDate();
    void setTime();
    uint8_t bcd_to_bin(uint8_t bcd);
    uint8_t bin_to_bcd(uint8_t in);

private:
    byte status_register;
    byte alarm_register;
    void writeAlarmControlRegister();
    // seed-generator related
};


#endif  // __in_PCF8583_h
