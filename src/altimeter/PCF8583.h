#ifndef __in_PCF8583_h
#define __in_PCF8583_h

#include <Arduino.h>
#include <Wire.h>

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
    short alarm_day;

    void enableSeedInterrupt();
    void disableSeedInterrupt();
    bool seedInterruptOccur();
    
    PCF8583();
    void begin();
    void init ();
    
    void get_time();
    void set_time();
    void get_alarm();

    void set_daily_alarm();
    short bcd_to_short(byte bcd);
    byte int_to_bcd(short in);

    void save_int(const byte &address, const int &data);
    int load_int(const byte &address);
private:
    byte status_register;
    byte readStatusRegister();
    byte alarm_register;
    void writeAlarmControlRegister();
    // seed-generator related
    byte seed_id; // : 1;
};


#endif  // __in_PCF8583_h
