#include "hwconfig.h"
#include "keyboard.h"

byte getKeypress(uint16_t timeout = 20000) {
    uint16_t i = 0;
    // Wait for all keys to be released
    for (;;) {
        if (!(BTN1_PRESSED || BTN2_PRESSED || BTN3_PRESSED))
            break;
        if ((++i) >= timeout)
            return 255;
        delay(15);
    }
    // Wait for keypress
    for (;;) {
        if (BTN1_PRESSED)
            return PIN_BTN1;
        if (BTN2_PRESSED)
            return PIN_BTN2;
        if (BTN3_PRESSED)
            return PIN_BTN3;
        if ((++i) >= timeout)
            break;
        delay(15);
    }

    return 255;
}
