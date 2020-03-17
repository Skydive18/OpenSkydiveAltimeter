#include "Arduino.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

volatile uint8_t disable_sleep = 0;

void powerDown() __attribute__((optimize("-O1")));
void powerDown() {
    ADCSRA &= ~(1 << ADEN); // ADC OFF
    do {
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        cli();
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
        sei();
    } while (0);
    ADCSRA |= (1 << ADEN); // ADC ON again
}
