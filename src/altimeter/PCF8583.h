#ifndef __in_PCF8583_h
#define __in_PCF8583_h

#include <Arduino.h>
#include <Wire.h>

class PCF8583 {
    short dow;
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

    PCF8583();
    void init ();
    
    void get_time();
    void set_time();
    void get_alarm();
    short get_day_of_week() const {
      return dow;
    }    

    void set_daily_alarm();
    short bcd_to_short(byte bcd);
    byte int_to_bcd(short in);
};


#endif  //PCF8583_H
