//#include <LowPower.h>
//void powerDown() { LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); }

#include "Arduino.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

volatile uint8_t disable_sleep = 0;

#ifndef sleep_bod_disable
#define sleep_bod_disable()                                         \
do {                                                                \
  unsigned char tempreg;                                                    \
  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t"             \
                       "ori %[tempreg], %[bods_bodse]" "\n\t"       \
                       "out %[mcucr], %[tempreg]" "\n\t"            \
                       "andi %[tempreg], %[not_bodse]" "\n\t"       \
                       "out %[mcucr], %[tempreg]"                   \
                       : [tempreg] "=&d" (tempreg)                  \
                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR),           \
                         [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)), \
                         [not_bodse] "i" (~_BV(BODSE)));            \
} while (0)
#endif

void powerDown() __attribute__((optimize("-O1")));
void powerDown() {
    ADCSRA &= ~(1 << ADEN); // ADC OFF
    do {
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        cli();
        sleep_enable();
        sleep_bod_disable();
        sei();
        sleep_cpu();
        sleep_disable();
        sei();
    } while (0);
    ADCSRA |= (1 << ADEN); // ADC ON again
}
