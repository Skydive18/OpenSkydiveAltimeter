#ifndef __in_RTC_h
#define __in_RTC_h

#include <Arduino.h>
#include <Wire.h>
#include "hwconfig.h"
#include "custom_types.h"

// RTC epoch year
#define EPOCH 2020

#if RTC==RTC_PCF8583
#define RTC_ADDRESS 0x50
// Addressing in RAM, altimeter runtime parameters
#define ADDR_YEAR_BASE 0x10
#define ADDR_LAST_STORED_YEAR 0x14
#define ADDR_ZERO_ALTITUDE 0x12

#elif RTC==RTC_PCF8563
#define RTC_ADDRESS 0x51
#define PCF8563_ALARM_REG_OFF  0x80
#define PCF8563_ALARM_AIE      0x02
#define PCF8563_ALARM_AF       0x08 // 0x08 : not 0x04!!!!
/* optional val for no alarm setting */
#define PCF8563_NO_ALARM       0x99

#endif

class Rtc {
public:
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;

#ifdef ALARM_ENABLE
    uint8_t alarm_minute;
    uint8_t alarm_hour;
    uint8_t alarm_enable;
    void enableAlarmInterrupt();
    void readAlarm();
    void setAlarm();
#endif

    void disableInterrupt();
    void disableHeartbeat();
    
    Rtc();
    void init();
    
    void readTime();
    timestamp_t getTimestamp();
    void setDate();
    void setTime();
    uint8_t bcd_to_bin(uint8_t bcd);
    uint8_t bin_to_bcd(uint8_t in);

    int16_t loadZeroAltitude();
    void saveZeroAltitude(int16_t zeroAltitude);

private:
#if RTC==RTC_PCF8583
    byte status_register;
    byte alarm_register;
    void writeAlarmControlRegister();
    uint16_t last_stored_year;
#elif RTC==RTC_PCF8563
    uint8_t status_register_1;
    uint8_t status_register_2;
#endif
};


#endif  // __in_RTC_h
