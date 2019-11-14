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
