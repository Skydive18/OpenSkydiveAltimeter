#include <Arduino.h>
#include "hwconfig.h"
#include "custom_types.h"

void initSound();
void termSound();
void sound(uint16_t frequency, uint16_t duration);
void noSound();
void LED_showOne(byte pin, byte val, byte disableSleepMask);

//#ifdef SOUND_PASSIVE
//#include "NewTone.h"
//#endif

#if defined(SOUND_ACTIVE) || defined(SOUND_PASSIVE)
//#include <MsTimer2.h>
#include "power.h"
//
//namespace {
//    volatile uint8_t sndmode = 0;
//    volatile uint8_t sndduration = 0;
//    volatile uint8_t sndptr = 0;
//
//    void pulseSound() {
//        if (sndduration > 0) {
//            sndduration--;
//            if (!sndduration)
//                noSound();
//        }
//    }
//}
#endif

#if defined(SOUND_EXTERNAL)
#include <Wire.h>
#endif

void initSound() {
#ifndef SOUND_EXTERNAL
    pinMode(PIN_SOUND, OUTPUT);
    digitalWrite(PIN_SOUND, 0);
#endif
//#if defined(SOUND_ACTIVE) || defined(SOUND_PASSIVE)
//    MsTimer2::set(100, pulseSound);
//    MsTimer2::start();
//#endif
}

void termSound() {
    noSound();
//#if defined(SOUND_ACTIVE) || defined(SOUND_PASSIVE)
//    MsTimer2::stop();
//#endif
}

void sound(uint16_t frequency, uint16_t duration) {
#if defined(SOUND_ACTIVE)
    digitalWrite(PIN_SOUND, 1);
//    if (duration > 0)
//        disable_sleep |= 0x2;
//    sndduration = duration;
#endif
#if defined(SOUND_PASSIVE)
//    if (duration > 0)
//        disable_sleep |= 0x2;
//    if (frequency)
//        NewTone(PIN_SOUND, frequency);
//    else {
        LED_showOne(PIN_SOUND, 128, 2);
//    }
#endif
}

void noSound() {
#if defined(SOUND_ACTIVE)
    disable_sleep &= 0xfd;
//    sndduration = 0;
#endif
//#if defined(SOUND_PASSIVE)
//    disable_sleep &= 0xfd;
//    sndduration = 0;
//    noNewTone(PIN_SOUND);
//#endif
#ifndef SOUND_EXTERNAL
    digitalWrite(PIN_SOUND, 0);
#endif
}

#if false

To hear the result connect a piezo speaker between pin D3 and ground (or D10 on ATmega32u4-based Arduinos).
I recommend getting a piezo speaker at least 20mm across for better quality sound.

ATmega328 version

Like the Arduino tone function, note uses Timer/Counter2 in the ATmega328, so pins D3 and D11
cannot be used for analogue PWM output at the same time.

Here's the definition of note:

const uint8_t scale[] PROGMEM = {239,226,213,201,190,179,169,160,151,142,134,127};

void note (int n, int octave) {
  DDRD = DDRD | 1<<DDD3;                     // PD3 (Arduino D3) as output
  TCCR2A = 0<<COM2A0 | 1<<COM2B0 | 2<<WGM20; // Toggle OC2B on match
  int prescaler = 9 - (octave + n/12);
  if (prescaler<3 || prescaler>6) prescaler = 0;
  OCR2A = pgm_read_byte(&scale[n % 12]) - 1;
  TCCR2B = 0<<WGM22 | prescaler<<CS20;
}

It takes advantage of the fact that the prescaler used in Timer/Counter2 provides four successive
powers of two: 32, 64, 128, and 256, so these can be used by the note function to set the octave
directly. The table of divisors for one octave is given by the array scale[], which is stored
in PROGMEM to avoid using up RAM.

ATmega32u4 version

On the ATmega32u4 note uses Timer/Counter4, so D6 and D13 cannot be used for analogue PWM output 
t the same time.

const uint8_t scale[] PROGMEM = {239,226,213,201,190,179,169,160,151,142,134,127};

void note (int n, int octave) {
  DDRB = DDRB | 1<<DDB6;                     // PB6 (Arduino D10) as output
  TCCR4A = 0<<COM4A0 | 1<<COM4B0;            // Toggle OC4B on match
  int prescaler = 10 - (octave + n/12);
  if (prescaler<1 || prescaler>9) prescaler = 0;
  OCR4C = pgm_read_byte(&scale[n % 12]) - 1;
  TCCR4B = prescaler<<CS40;
}

Timer/Counter4 on the ATmega32u4 provides a full range of prescaler divisors, allowing this version
of note to have a wider range of octaves.
#endif

#if false

/*
  MsTimer2.h - Using timer2 with 1ms resolution
  Javier Valencia <javiervalencia80@gmail.com>

  https://github.com/PaulStoffregen/MsTimer2
  
  History:
    6/Jun/14  - V0.7 added support for Teensy 3.0 & 3.1
    29/Dec/11 - V0.6 added support for ATmega32u4, AT90USB646, AT90USB1286 (paul@pjrc.com)
        some improvements added by Bill Perry
        note: uses timer4 on Atmega32u4
    29/May/09 - V0.5 added support for Atmega1280 (thanks to Manuel Negri)
    19/Mar/09 - V0.4 added support for ATmega328P (thanks to Jerome Despatis)
    11/Jun/08 - V0.3 
        changes to allow working with different CPU frequencies
        added support for ATMega128 (using timer2)
        compatible with ATMega48/88/168/8
    10/May/08 - V0.2 added some security tests and volatile keywords
    9/May/08 - V0.1 released working on ATMEGA168 only
    

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <MsTimer2.h>

unsigned long MsTimer2::msecs;
void (*MsTimer2::func)();
volatile unsigned long MsTimer2::count;
volatile char MsTimer2::overflowing;
volatile unsigned int MsTimer2::tcnt2;
#if defined(__arm__) && defined(TEENSYDUINO)
static IntervalTimer itimer;
#endif

void MsTimer2::set(unsigned long ms, void (*f)()) {
    float prescaler = 0.0;
    
    if (ms == 0)
        msecs = 1;
    else
        msecs = ms;
        
    func = f;

#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TIMSK2 &= ~(1<<TOIE2);
    TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
    TCCR2B &= ~(1<<WGM22);
    ASSR &= ~(1<<AS2);
    TIMSK2 &= ~(1<<OCIE2A);
    
    if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {    // prescaler set to 64
        TCCR2B |= (1<<CS22);
        TCCR2B &= ~((1<<CS21) | (1<<CS20));
        prescaler = 64.0;
    } else if (F_CPU < 1000000UL) { // prescaler set to 8
        TCCR2B |= (1<<CS21);
        TCCR2B &= ~((1<<CS22) | (1<<CS20));
        prescaler = 8.0;
    } else { // F_CPU > 16Mhz, prescaler set to 128
        TCCR2B |= ((1<<CS22) | (1<<CS20));
        TCCR2B &= ~(1<<CS21);
        prescaler = 128.0;
    }
#elif defined (__AVR_ATmega8__)
    TIMSK &= ~(1<<TOIE2);
    TCCR2 &= ~((1<<WGM21) | (1<<WGM20));
    TIMSK &= ~(1<<OCIE2);
    ASSR &= ~(1<<AS2);
    
    if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {    // prescaler set to 64
        TCCR2 |= (1<<CS22);
        TCCR2 &= ~((1<<CS21) | (1<<CS20));
        prescaler = 64.0;
    } else if (F_CPU < 1000000UL) { // prescaler set to 8
        TCCR2 |= (1<<CS21);
        TCCR2 &= ~((1<<CS22) | (1<<CS20));
        prescaler = 8.0;
    } else { // F_CPU > 16Mhz, prescaler set to 128
        TCCR2 |= ((1<<CS22) && (1<<CS20));
        TCCR2 &= ~(1<<CS21);
        prescaler = 128.0;
    }
#elif defined (__AVR_ATmega128__)
    TIMSK &= ~(1<<TOIE2);
    TCCR2 &= ~((1<<WGM21) | (1<<WGM20));
    TIMSK &= ~(1<<OCIE2);
    
    if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {    // prescaler set to 64
        TCCR2 |= ((1<<CS21) | (1<<CS20));
        TCCR2 &= ~(1<<CS22);
        prescaler = 64.0;
    } else if (F_CPU < 1000000UL) { // prescaler set to 8
        TCCR2 |= (1<<CS21);
        TCCR2 &= ~((1<<CS22) | (1<<CS20));
        prescaler = 8.0;
    } else { // F_CPU > 16Mhz, prescaler set to 256
        TCCR2 |= (1<<CS22);
        TCCR2 &= ~((1<<CS21) | (1<<CS20));
        prescaler = 256.0;
    }
#elif defined (__AVR_ATmega32U4__)
    TCCR4B = 0;
    TCCR4A = 0;
    TCCR4C = 0;
    TCCR4D = 0;
    TCCR4E = 0;
    if (F_CPU >= 16000000L) {
        TCCR4B = (1<<CS43) | (1<<PSR4);
        prescaler = 128.0;
    } else if (F_CPU >= 8000000L) {
        TCCR4B = (1<<CS42) | (1<<CS41) | (1<<CS40) | (1<<PSR4);
        prescaler = 64.0;
    } else if (F_CPU >= 4000000L) {
        TCCR4B = (1<<CS42) | (1<<CS41) | (1<<PSR4);
        prescaler = 32.0;
    } else if (F_CPU >= 2000000L) {
        TCCR4B = (1<<CS42) | (1<<CS40) | (1<<PSR4);
        prescaler = 16.0;
    } else if (F_CPU >= 1000000L) {
        TCCR4B = (1<<CS42) | (1<<PSR4);
        prescaler = 8.0;
    } else if (F_CPU >= 500000L) {
        TCCR4B = (1<<CS41) | (1<<CS40) | (1<<PSR4);
        prescaler = 4.0;
    } else {
        TCCR4B = (1<<CS41) | (1<<PSR4);
        prescaler = 2.0;
    }
    tcnt2 = (int)((float)F_CPU * 0.001 / prescaler) - 1;
    OCR4C = tcnt2;
    return;
#elif defined(__arm__) && defined(TEENSYDUINO)
    // nothing needed here
#else
#error Unsupported CPU type
#endif

    tcnt2 = 256 - (int)((float)F_CPU * 0.001 / prescaler);
}

void MsTimer2::start() {
    count = 0;
    overflowing = 0;
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TCNT2 = tcnt2;
    TIMSK2 |= (1<<TOIE2);
#elif defined (__AVR_ATmega128__)
    TCNT2 = tcnt2;
    TIMSK |= (1<<TOIE2);
#elif defined (__AVR_ATmega8__)
    TCNT2 = tcnt2;
    TIMSK |= (1<<TOIE2);
#elif defined (__AVR_ATmega32U4__)
    TIFR4 = (1<<TOV4);
    TCNT4 = 0;
    TIMSK4 = (1<<TOIE4);
#elif defined(__arm__) && defined(TEENSYDUINO)
    itimer.begin(MsTimer2::_overflow, 1000);
#endif
}

void MsTimer2::stop() {
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TIMSK2 &= ~(1<<TOIE2);
#elif defined (__AVR_ATmega128__)
    TIMSK &= ~(1<<TOIE2);
#elif defined (__AVR_ATmega8__)
    TIMSK &= ~(1<<TOIE2);
#elif defined (__AVR_ATmega32U4__)
    TIMSK4 = 0;
#elif defined(__arm__) && defined(TEENSYDUINO)
    itimer.end();
#endif
}

void MsTimer2::_overflow() {
    count += 1;
    
    if (count >= msecs && !overflowing) {
        overflowing = 1;
        count = count - msecs; // subtract ms to catch missed overflows
                    // set to 0 if you don't want this.
        (*func)();
        overflowing = 0;
    }
}

#if defined (__AVR__)
#if defined (__AVR_ATmega32U4__)
ISR(TIMER4_OVF_vect) {
#else
ISR(TIMER2_OVF_vect) {
#endif
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TCNT2 = MsTimer2::tcnt2;
#elif defined (__AVR_ATmega128__)
    TCNT2 = MsTimer2::tcnt2;
#elif defined (__AVR_ATmega8__)
    TCNT2 = MsTimer2::tcnt2;
#elif defined (__AVR_ATmega32U4__)
    // not necessary on 32u4's high speed timer4
#endif
    MsTimer2::_overflow();
}
#endif // AVR




#endif
