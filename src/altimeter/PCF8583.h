#ifndef __in_PCF8583_h
#define __in_PCF8583_h

#include <Arduino.h>
#include <Wire.h>

// Addressing in RAM, altimeter runtime parameters
#define ADDR_ZERO_ALTITUDE 0x12
#define ADDR_POWERMODE 0x14

#define RTC_ADDRESS 0x50

class PCF8583 {
  public:
    short ssecond;
    short second;
    short minute;
    short hour;
    short day;
    short month;
    int year;
    int year_base;

    short alarm_second;
    short alarm_minute;
    short alarm_hour;

    void enableSeedInterrupt();
    void disableSeedInterrupt();
    
    PCF8583();
    void begin();
    void init ();
    
    void get_time();
    void set_time();
    void get_alarm();

    void set_daily_alarm();
    short bcd_to_short(byte bcd);
    byte int_to_bcd(short in);

private:
    byte status_register;
    byte alarm_register;
    void writeAlarmControlRegister();
    // seed-generator related
};


#endif  // __in_PCF8583_h
