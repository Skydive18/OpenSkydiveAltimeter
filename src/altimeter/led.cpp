#include <Arduino.h>
#include "hwconfig.h"
#include "led.h"

#ifdef LED_COMMON_CATHODE
void LED_showOne(byte pin, byte color) {
    if (color == 0) {
        digitalWrite(pin, 0);
    } else if (color == 255) {
        digitalWrite(pin, 1);
    } else {
        analogWrite(pin, color);
    }
}
#else
#ifdef LED_COMMON_ANODE
void LED_showOne(byte pin, byte color) {
    if (color == 0) {
        digitalWrite(pin, 1);
    } else if (color == 255) {
        digitalWrite(pin, 0);
    } else {
        analogWrite(pin, 255-color);
    }
}
#endif
#endif

void LED_show(byte red, byte green, byte blue, int delayMs = 0) {
    LED_showOne(PIN_R, red);
    LED_showOne(PIN_G, green);
    LED_showOne(PIN_B, blue);
    if (delayMs > 0) {
        delay(delayMs);
        LED_showOne(PIN_R, 0);
        LED_showOne(PIN_G, 0);
        LED_showOne(PIN_B, 0);
    }
}

void LED_init() {
    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    LED_show(0, 0, 0);
}
