#ifndef __in_powerctl_h
#define __in_powerctl_h

#include <LowPower.h>

#define off_15ms        LowPower.powerDown(SLEEP_15MS,    ADC_OFF, BOD_OFF)
#define off_30ms        LowPower.powerDown(SLEEP_30MS,    ADC_OFF, BOD_OFF)
#define off_60ms        LowPower.powerDown(SLEEP_60MS,    ADC_OFF, BOD_OFF)
#define off_120ms       LowPower.powerDown(SLEEP_120MS,   ADC_OFF, BOD_OFF)
#define off_250ms       LowPower.powerDown(SLEEP_250MS,   ADC_OFF, BOD_OFF)
#define off_500ms       LowPower.powerDown(SLEEP_500MS,   ADC_OFF, BOD_OFF)
#define off_1s          LowPower.powerDown(SLEEP_1S,      ADC_OFF, BOD_OFF)
#define off_2s          LowPower.powerDown(SLEEP_2S,      ADC_OFF, BOD_OFF)
//#define off_2s sleepMode(SLEEP_POWER_DOWN); sleep();
#define off_4s          LowPower.powerDown(SLEEP_4S,      ADC_OFF, BOD_OFF)
#define off_8s          LowPower.powerDown(SLEEP_8S,      ADC_OFF, BOD_OFF)
#define off_forever     LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF)
//#define off_forever sleepMode(SLEEP_POWER_DOWN); sleep();

#define pwsave_15ms     LowPower.powerSave(SLEEP_15MS,    ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_30ms     LowPower.powerSave(SLEEP_30MS,    ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_60ms     LowPower.powerSave(SLEEP_60MS,    ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_120ms    LowPower.powerSave(SLEEP_120MS,   ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_250ms    LowPower.powerSave(SLEEP_250MS,   ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_500ms    LowPower.powerSave(SLEEP_500MS,   ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_1s       LowPower.powerSave(SLEEP_1S,      ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_2s       LowPower.powerSave(SLEEP_2S,      ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_4s       LowPower.powerSave(SLEEP_4S,      ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_8s       LowPower.powerSave(SLEEP_8S,      ADC_OFF, BOD_OFF, TIMER2_ON)
#define pwsave_forever  LowPower.powerSave(SLEEP_FOREVER, ADC_OFF, BOD_OFF, TIMER2_ON)

#define standby_15ms    LowPower.powerStandby(SLEEP_15MS,    ADC_OFF, BOD_OFF)
#define standby_30ms    LowPower.powerStandby(SLEEP_30MS,    ADC_OFF, BOD_OFF)
#define standby_60ms    LowPower.powerStandby(SLEEP_60MS,    ADC_OFF, BOD_OFF)
#define standby_120ms   LowPower.powerStandby(SLEEP_120MS,   ADC_OFF, BOD_OFF)
#define standby_250ms   LowPower.powerStandby(SLEEP_250MS,   ADC_OFF, BOD_OFF)
#define standby_500ms   LowPower.powerStandby(SLEEP_500MS,   ADC_OFF, BOD_OFF)
#define standby_1s      LowPower.powerStandby(SLEEP_1S,      ADC_OFF, BOD_OFF)
#define standby_2s      LowPower.powerStandby(SLEEP_2S,      ADC_OFF, BOD_OFF)
#define standby_4s      LowPower.powerStandby(SLEEP_4S,      ADC_OFF, BOD_OFF)
#define standby_8s      LowPower.powerStandby(SLEEP_8S,      ADC_OFF, BOD_OFF)
#define standby_forever LowPower.powerStandby(SLEEP_FOREVER, ADC_OFF, BOD_OFF)

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega168__)
#define idle_4s LowPower.idle(SLEEP_4S, ADC_OFF, TIMER2_ON, TIMER1_ON, TIMER0_ON, SPI_ON, USART0_ON, TWI_ON)
#elif defined __AVR_ATmega32U4__    
#define idle_4s LowPower.idle(SLEEP_4S, ADC_OFF, TIMER4_ON, TIMER3_ON, TIMER1_ON, TIMER0_ON, SPI_OFF, USART1_OFF, TWI_OFF, USB_OFF);     
#endif

extern uint8_t disable_sleep;

#endif
