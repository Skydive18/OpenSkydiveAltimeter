#ifndef __in_powerctl_h
#define __in_powerctl_h

#include <LowPower.h>

#define off_2s          LowPower.powerDown(SLEEP_2S,      ADC_OFF, BOD_OFF)
//#define off_2s sleepMode(SLEEP_POWER_DOWN); sleep();
#define off_forever     LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF)
//#define off_forever sleepMode(SLEEP_POWER_DOWN); sleep();

extern uint8_t disable_sleep;

#endif
