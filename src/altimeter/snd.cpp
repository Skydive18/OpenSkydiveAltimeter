#include <Arduino.h>
#include "hwconfig.h"
#include "custom_types.h"

void initSound();
void termSound();
void sound(uint16_t frequency, uint8_t duration);
void noSound();

#ifdef SOUND_PASSIVE
#include "NewTone.h"
#endif

#if defined(SOUND_ACTIVE) || defined(SOUND_PASSIVE)
#include <MsTimer2.h>
#include "power.h"

namespace {
    volatile uint8_t sndmode = 0;
    volatile uint8_t sndduration = 0;
    volatile uint8_t sndptr = 0;

    void pulseSound() {
        if (sndduration > 0) {
            sndduration--;
            if (!sndduration)
                noSound();
        }
    }
}
#endif

#if defined(SOUND_EXTERNAL)
#include <Wire.h>
#endif

void initSound() {
#ifndef SOUND_EXTERNAL
    pinMode(PIN_SOUND, OUTPUT);
    digitalWrite(PIN_SOUND, 0);
#endif
#if defined(SOUND_ACTIVE) || defined(SOUND_PASSIVE)
    MsTimer2::set(100, pulseSound);
    MsTimer2::start();
#endif
}

void termSound() {
    noSound();
#if defined(SOUND_ACTIVE) || defined(SOUND_PASSIVE)
    MsTimer2::stop();
#endif
}

void sound(uint16_t frequency, uint8_t duration) {
#if defined(SOUND_ACTIVE)
    digitalWrite(PIN_SOUND, 1);
    if (duration > 0)
        disable_sleep |= 0x2;
    sndduration = duration;
#endif
#if defined(SOUND_PASSIVE)
    if (duration > 0)
        disable_sleep |= 0x2;
    NewTone(PIN_SOUND, frequency);
#endif
}

void noSound() {
#if defined(SOUND_ACTIVE)
    disable_sleep &= 0xfd;
    sndduration = 0;
#endif
#if defined(SOUND_PASSIVE)
    disable_sleep &= 0xfd;
    sndduration = 0;
    noNewTone(PIN_SOUND);
#endif
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
