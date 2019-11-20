#ifndef __in_snd_h
#define __in_snd_h
#include "hwconfig.h"

void initSound();
void termSound();
void noSound();

#if defined(SOUND_PASSIVE) || defined(SOUND_ACTIVE) || defined(SOUND_EXTERNAL)
void sound(uint16_t frequency, uint8_t duration);
#else
#define sound(a,b)
#endif

#endif

#if false

#ifndef MsTimer2_h
#define MsTimer2_h

#ifdef __AVR__
#include <avr/interrupt.h>
#elif defined(__arm__) && defined(TEENSYDUINO)
#include <Arduino.h>
#else
#error MsTimer2 library only works on AVR architecture
#endif

namespace MsTimer2 {
    extern unsigned long msecs;
    extern void (*func)();
    extern volatile unsigned long count;
    extern volatile char overflowing;
    extern volatile unsigned int tcnt2;
    
    void set(unsigned long ms, void (*f)());
    void start();
    void stop();
    void _overflow();
}

#endif


#endif
