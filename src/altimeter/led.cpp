#include <Arduino.h>
#include "hwconfig.h"
#include "led.h"
#include "wiring_private.h"

volatile bool pwm_active = false;

void LED_showOne(byte pin, byte val) {
    // We need to make sure the PWM output is enabled for those pins
    // that support it, as we turn it off when digitally reading or
    // writing with them.  Also, make sure the pin is in output mode
    // for consistenty with Wiring, which doesn't require a pinMode
    // call for the analog output pins.
    pinMode(pin, OUTPUT);
#ifdef LED_COMMON_ANODE
    val = 255 - val;
#endif
    if (val == 0)
    {
        digitalWrite(pin, LOW);
    }
    else if (val == 255)
    {
        digitalWrite(pin, HIGH);
    }
    else
    {
        pwm_active = true;
        
        switch(digitalPinToTimer(pin))
        {
            // XXX fix needed for atmega8
            #if defined(TCCR0) && defined(COM00) && !defined(__AVR_ATmega8__)
            case TIMER0A:
                // connect pwm to pin on timer 0
                sbi(TCCR0, COM00);
                OCR0 = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR0A) && defined(COM0A1)
            case TIMER0A:
                // connect pwm to pin on timer 0, channel A
                sbi(TCCR0A, COM0A1);
                OCR0A = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR0A) && defined(COM0B1)
            case TIMER0B:
                // connect pwm to pin on timer 0, channel B
                sbi(TCCR0A, COM0B1);
                OCR0B = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR1A) && defined(COM1A1)
            case TIMER1A:
                // connect pwm to pin on timer 1, channel A
                sbi(TCCR1A, COM1A1);
                OCR1A = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR1A) && defined(COM1B1)
            case TIMER1B:
                // connect pwm to pin on timer 1, channel B
                sbi(TCCR1A, COM1B1);
                OCR1B = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR1A) && defined(COM1C1)
            case TIMER1C:
                // connect pwm to pin on timer 1, channel B
                sbi(TCCR1A, COM1C1);
                OCR1C = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR2) && defined(COM21)
            case TIMER2:
                // connect pwm to pin on timer 2
                sbi(TCCR2, COM21);
                OCR2 = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR2A) && defined(COM2A1)
            case TIMER2A:
                // connect pwm to pin on timer 2, channel A
                sbi(TCCR2A, COM2A1);
                OCR2A = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR2A) && defined(COM2B1)
            case TIMER2B:
                // connect pwm to pin on timer 2, channel B
                sbi(TCCR2A, COM2B1);
                OCR2B = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR3A) && defined(COM3A1)
            case TIMER3A:
                // connect pwm to pin on timer 3, channel A
                sbi(TCCR3A, COM3A1);
                OCR3A = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR3A) && defined(COM3B1)
            case TIMER3B:
                // connect pwm to pin on timer 3, channel B
                sbi(TCCR3A, COM3B1);
                OCR3B = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR3A) && defined(COM3C1)
            case TIMER3C:
                // connect pwm to pin on timer 3, channel C
                sbi(TCCR3A, COM3C1);
                OCR3C = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR4A)
            case TIMER4A:
                //connect pwm to pin on timer 4, channel A
                sbi(TCCR4A, COM4A1);
                #if defined(COM4A0)     // only used on 32U4
                cbi(TCCR4A, COM4A0);
                #endif
                OCR4A = val;    // set pwm duty
                break;
            #endif
            
            #if defined(TCCR4A) && defined(COM4B1)
            case TIMER4B:
                // connect pwm to pin on timer 4, channel B
                sbi(TCCR4A, COM4B1);
                OCR4B = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR4A) && defined(COM4C1)
            case TIMER4C:
                // connect pwm to pin on timer 4, channel C
                sbi(TCCR4A, COM4C1);
                OCR4C = val; // set pwm duty
                break;
            #endif
                
            #if defined(TCCR4C) && defined(COM4D1)
            case TIMER4D:               
                // connect pwm to pin on timer 4, channel D
                sbi(TCCR4C, COM4D1);
                #if defined(COM4D0)     // only used on 32U4
                cbi(TCCR4C, COM4D0);
                #endif
                OCR4D = val;    // set pwm duty
                break;
            #endif

                            
            #if defined(TCCR5A) && defined(COM5A1)
            case TIMER5A:
                // connect pwm to pin on timer 5, channel A
                sbi(TCCR5A, COM5A1);
                OCR5A = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR5A) && defined(COM5B1)
            case TIMER5B:
                // connect pwm to pin on timer 5, channel B
                sbi(TCCR5A, COM5B1);
                OCR5B = val; // set pwm duty
                break;
            #endif

            #if defined(TCCR5A) && defined(COM5C1)
            case TIMER5C:
                // connect pwm to pin on timer 5, channel C
                sbi(TCCR5A, COM5C1);
                OCR5C = val; // set pwm duty
                break;
            #endif

        }
    }
}

void LED_show(byte red, byte green, byte blue, uint8_t delayMs = 0) {
    pwm_active = false;
    LED_showOne(PIN_R, red);
    LED_showOne(PIN_G, green);
    LED_showOne(PIN_B, blue);
    if (delayMs > 0) {
        delay(delayMs);
        LED_showOne(PIN_R, 0);
        LED_showOne(PIN_G, 0);
        LED_showOne(PIN_B, 0);
        pwm_active = false;
    }
}

bool IsPWMActive() {
    return pwm_active;
}
