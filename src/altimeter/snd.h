#ifndef __in_snd_h
#define __in_snd_h
#include "hwconfig.h"

void initSound();
void termSound();
void noSound();

// Signals:
// 0 Buzz until termination
// 1 Alarm buzz: 100ms sound + 100ms delay, twice

#if defined(SOUND_PASSIVE) || defined(SOUND_ACTIVE) || defined(SOUND_EXTERNAL)
void sound(uint8_t signalNumber);
#else
#define sound(a)
#endif

#if defined(SOUND_PASSIVE) || defined(SOUND_ACTIVE)
// Use timer
#ifdef __AVR__
#include <avr/interrupt.h>
#elif defined(__arm__) && defined(TEENSYDUINO)
#include <Arduino.h>
#else
#error MsTimer2m library only works on AVR architecture
#endif

#define SOUND_USE_TIMER

namespace MsTimer2m {
    extern volatile uint8_t tcnt2;
    extern volatile uint16_t count;
    extern volatile uint16_t duration;
    volatile uint8_t sndpos;
    volatile uint16_t* sndptr;
    
    void set(uint16_t tick_frequncy, bool silent);
    void stop();
    void nextNote();
}

#endif // defined(SOUND_PASSIVE) || defined(SOUND_ACTIVE)

#endif
