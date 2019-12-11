#ifndef __in_snd_h
#define __in_snd_h
#include "hwconfig.h"

void initSound();
void noSound();

// Signals:
#define SIGNAL_WELCOME 2
#define SIGNAL_2SHORT 1
#define SIGNAL_1MEDIUM 3

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
#error Timer-based sound subsystem sypported on AVR architecture only.
#endif

#endif // defined(SOUND_PASSIVE) || defined(SOUND_ACTIVE)

#endif
