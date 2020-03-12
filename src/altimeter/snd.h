#ifndef __in_snd_h
#define __in_snd_h
#include "hwconfig.h"

void initSound();
void noSound();

// Signals:
#define SIGNAL_WELCOME 2
#define SIGNAL_2SHORT 1
#define SIGNAL_1MEDIUM 3

#if defined(SOUND)
void sound(uint8_t signalNumber);

// Use timer
#ifdef __AVR__
#include <avr/interrupt.h>
#elif defined(__arm__) && defined(TEENSYDUINO)
#include <Arduino.h>
#else
#error Timer-based sound subsystem supported on AVR architecture only.
#endif

#else

#define sound(a)

#endif

#ifdef SOUND_VOLUME_CONTROL_ENABLE
void setVol(uint8_t vol);
#define SET_MAX_VOL setVol(3)
#define SET_VOL setVol(settings.volume)
#else
#define SET_MAX_VOL
#define SET_VOL
#endif

#endif
