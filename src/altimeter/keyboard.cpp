#include "hwconfig.h"
#include "keyboard.h"
#include "common.h"

extern settings_t settings;

uint8_t getKeypress(uint16_t timeout = 20000) {
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
            return (settings.display_rotation & 1) ? PIN_BTN1 : PIN_BTN3;
        if (BTN2_PRESSED)
            return PIN_BTN2;
        if (BTN3_PRESSED)
            return (settings.display_rotation & 1) ? PIN_BTN3 : PIN_BTN1;
        if ((++i) >= timeout)
            break;
        delay(15);
    }

    return 255;
}
