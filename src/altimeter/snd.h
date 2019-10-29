#ifndef __in_snd_h
#define __in_snd_h
#include <Arduino.h>

void initSound();
void termSound();
void sound(uint16_t frequency, uint8_t duration = 0);
void noSound();
#endif
