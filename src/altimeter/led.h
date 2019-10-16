#ifndef __in_led_h
#define __in_led_h

void LED_show(byte red, byte green, byte blue, int delayMs = 0);

bool IsPWMActive();

#endif
