#include <Arduino.h>
#include "hwconfig.h"
#include "custom_types.h"
#include "snd.h"

void initSound();
void termSound();
void sound(uint8_t signalNumber);
void noSound();

#ifdef SOUND_USE_TIMER
#include "power.h"
#endif

#ifdef SOUND_VOLUME_CONTROL_ENABLE
#include "common.h"
extern settings_t settings;
void setVol(uint8_t vol) {
    IIC_WriteByte(SOUND_VOLUME_CONTROL_ADDR, settings.volumemap[vol & 3]);
}
#endif

void initSound() {
    pinMode(PIN_SOUND, OUTPUT);
    digitalWrite(PIN_SOUND, 0);
}

#ifdef SOUND_USE_TIMER

namespace MsTimer2m {
    volatile uint16_t duration;
    volatile uint8_t* sndptr;
    
    void stop();
    void nextNote();

#if SOUND==SOUND_PASSIVE
//                               C    C#   D    D#   E    F    F#   G    G#   A    A#   B
const uint8_t scale[] PROGMEM = {239, 226, 213, 201, 190, 179, 169, 160, 151, 142, 134, 127};
const uint8_t freq[]  PROGMEM = {105, 111, 118, 125, 132, 140, 149, 157, 167, 174, 188, 194 };

// Signal templates. Contain note numbers (1-12) + (octave*12) and durations in 50ms ticks. 255 for pause, 0 terminates.
const uint8_t signal_2short[] PROGMEM = {29, 2, 255, 2, 29, 2, 0 };
const uint8_t signal_1medium[] PROGMEM = {29, 6, 0 };
#ifdef GREETING_ENABLE
const uint8_t sineva[] PROGMEM = {
40,8,   38,8,   40,8,   38,16,  36,4,   35,4,   33,16,  255,8,  38,4,   36,4,   38,8,   36,4,   35,20,
255,24, 35,4,   38,4,   36,12,  35,4,   33,12,  31,4,   30,16,  35,8,   33,8,   35,12,  28,30,  0
};
#endif
//
#ifdef AUDIBLE_SIGNALS_ENABLE
const uint8_t signal_alt0[] PROGMEM = {42,3, 255,2, 42,3, 255,2, 42,3, 255,2, 42,3, 0 };
const uint8_t signal_alt1[] PROGMEM = {38,4, 255,2, 38,4, 255,2, 38,4, 0 };
const uint8_t signal_alt2[] PROGMEM = {35,5, 255,3, 35,5, 0 };
const uint8_t signal_alt3[] PROGMEM = {42,3, 35,3, 42,3, 35,3, 42,3, 35,3, 42,3, 35,3, 42,3, 35,3, 0 };
const uint8_t signal_alt4[] PROGMEM = {42,2, 255,1, 42,2, 255,6, 42,2, 255,1, 42,2, 255,6, 42,2, 255,1, 42,2, 255,6, 42,2, 255,1, 42,2, 0 };
const uint8_t signal_alt5[] PROGMEM = {38,2, 255,1, 38,2, 255,6, 38,2, 255,1, 38,2, 255,6, 38,2, 255,1, 38,2, 0 };
const uint8_t signal_alt6[] PROGMEM = {35,2, 255,1, 35,2, 255,6, 35,2, 255,1, 35,2, 0 };
const uint8_t signal_alt7[] PROGMEM = {32,50, 0 };
#endif

void note(uint8_t noteNumber) {
    bool silent = false;
    if (noteNumber > 250) {
        silent = true;
        noteNumber = 11;
    }
#if defined(__AVR_ATmega32U4__)
    TCCR4C = (1 << PWM4D) | (1 << COM4D1); // enable output
    uint8_t note = noteNumber % 12;
    uint8_t octave = (noteNumber/12);
    uint8_t prescaler = 9 - octave;
    uint8_t val = pgm_read_byte(&scale[note]);
    OCR4C = val;
    OCR4D = silent ? 0 : (val / 2);
    TCCR4B = prescaler<<CS40;
    duration = (duration * pgm_read_byte(&freq[note])) >> (5 - octave);
    // Start
    disable_sleep |= 0x2;
    TIFR4 = (1<<TOV4);
    TCNT4 = 0;
    TIMSK4 = (1<<TOIE4);
#elif defined (__AVR_ATmega328P__) || defined(__AVR_ATmega2560__)
//    DDRD = DDRD | 1<<DDD3;                     // PD3 (Arduino D3) as output
    uint8_t note = noteNumber % 12;
    uint8_t octave = (noteNumber/12) + 1;
    uint8_t prescaler = 7 - octave;
    TCCR2A = 0<<COM2A0 | (silent ? 0: 1) <<COM2B0 | 2<<WGM20; // Toggle OC2B on match
    TCCR2B = 0<<WGM22 | prescaler<<CS20;
    TCNT2 = 0;
    OCR2A = pgm_read_byte(&scale[note]) - 1;
    duration = ((duration) * pgm_read_byte(&freq[note])) >> (5 - octave);
    // Start
    disable_sleep |= 0x2;
    TIMSK2 |= (1 << OCIE2B);
#endif
}

void nextNote() {
    uint8_t freq = pgm_read_byte(MsTimer2m::sndptr++);
    if (freq) {
        MsTimer2m::duration = (uint16_t)pgm_read_byte(MsTimer2m::sndptr++);
        MsTimer2m::note(freq - 1);
        return;
    }
    noSound();
}

#elif SOUND==SOUND_ACTIVE
volatile uint8_t buzz;
// Signal templates. Contain durations for beeps and pauses, sequentially, in 50ms ticks. Terminated with 0.
const uint8_t signal_2short[] PROGMEM = {2, 2, 2, 0 };
const uint8_t signal_1medium[] PROGMEM = {6, 0 };
#ifdef AUDIBLE_SIGNALS_ENABLE
const uint8_t signal_alt0[] PROGMEM = {3, 2, 3, 2, 3, 2, 3, 0 };
const uint8_t signal_alt1[] PROGMEM = {4, 2, 4, 2, 4, 0 };
const uint8_t signal_alt2[] PROGMEM = {4, 3, 4, 0 };
const uint8_t signal_alt3[] PROGMEM = {6, 1, 6, 1, 6, 1, 6, 1, 6, 0 };
const uint8_t signal_alt4[] PROGMEM = {1, 1, 1, 6, 1, 1, 1, 6, 1, 1, 1, 6, 1, 1, 1, 0 };
const uint8_t signal_alt5[] PROGMEM = {1, 1, 1, 6, 1, 1, 1, 6, 1, 1, 1, 0 };
const uint8_t signal_alt6[] PROGMEM = {1, 1, 1, 6, 1, 1, 1, 0 };
const uint8_t signal_alt7[] PROGMEM = {40, 0 };
#endif
void note() {
#if defined(__AVR_ATmega32U4__)
    TCCR4C = 0; // disable timer output
    OCR4C = 127;
    OCR4D = 0;
    TCCR4B = 9<<CS40;
    duration = (duration * 194) >> 5;
    digitalWrite(PIN_SOUND, (++buzz) & 1);
    // Start
    disable_sleep |= 0x2;
    TIFR4 = (1<<TOV4);
    TCNT4 = 0;
    TIMSK4 = (1<<TOIE4);
#elif defined (__AVR_ATmega328P__) || defined(__AVR_ATmega2560__)
    TCNT2 = 0;
    OCR2A = 127;
    TCCR2A = 2<<WGM20; // Clear-on-compare-match mode, no output
    TCCR2B = 0<<WGM22 | 6<<CS20;
    duration = (duration * 194) >> 3;
    digitalWrite(PIN_SOUND, (++buzz) & 1);
    // Start
    disable_sleep |= 0x2;
    TIMSK2 |= (1 << OCIE2B);
#endif
}

void nextNote() {
    noSound();
    MsTimer2m::duration = (uint16_t)pgm_read_byte(MsTimer2m::sndptr);
    if (MsTimer2m::duration) {
        MsTimer2m::sndptr++;
        MsTimer2m::note();
    }
}

#endif // SOUND_ACTIVE

} // namespace

void MsTimer2m::stop() {
#if defined (__AVR_ATmega328P__) || defined(__AVR_ATmega2560__)
    TIMSK2 &= ~(1<<OCIE2B);
    TCCR2A &= ~((1 << COM2B0) | (1 << COM2B1)); // Disable OCR2B output
#elif defined (__AVR_ATmega32U4__)
    TIMSK4 = 0;
#endif
    disable_sleep &= 0xfd;
    digitalWrite(PIN_SOUND, 0);
}

#if defined (__AVR__)
#if defined (__AVR_ATmega32U4__)
ISR(TIMER4_OVF_vect) {
#else
ISR(TIMER2_COMPB_vect) {
#endif
    if (! (--MsTimer2m::duration)) {
        MsTimer2m::stop();
        MsTimer2m::nextNote();
    }
}
#endif // AVR

#endif // SOUND_USE_TIMER

void sound(uint8_t signalNumber) {
#if defined(SOUND_USE_TIMER)
    noSound(); // Stop current generation, if any.
#if SOUND==SOUND_ACTIVE
    MsTimer2m::buzz = 0; // sequence will start from buzzing, not from pause
#endif
    if (signalNumber == SIGNAL_2SHORT) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_2short;
    } else if (signalNumber == SIGNAL_1MEDIUM) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_1medium;
#ifdef AUDIBLE_SIGNALS_ENABLE
    } else if (signalNumber == 128) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt0;
    } else if (signalNumber == 129) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt1;
    } else if (signalNumber == 130) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt2;
    } else if (signalNumber == 131) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt3;
    } else if (signalNumber == 132) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt4;
    } else if (signalNumber == 133) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt5;
    } else if (signalNumber == 134) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt6;
    } else if (signalNumber == 135) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::signal_alt7;
#endif
#if SOUND==SOUND_PASSIVE && defined(GREETING_ENABLE)
    } else if (signalNumber == SIGNAL_WELCOME) {
        MsTimer2m::sndptr = (volatile uint8_t*)MsTimer2m::sineva; 
#endif
    } else
        return; // Do nothing for unsupported sequences
    MsTimer2m::nextNote();
#endif
}

void noSound() {
#if defined(SOUND_USE_TIMER)
    MsTimer2m::stop();
    disable_sleep &= 0xfd;
#endif
}
