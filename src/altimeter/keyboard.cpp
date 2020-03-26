#include "hwconfig.h"
#include "keyboard.h"
#include "common.h"

extern settings_t settings;

#define KBD_TIMEOUT 20000
bool auto_repeat;
uint8_t getKeypress(bool enable_repeat) {
    uint16_t i = 0;
    // Wait for all keys to be released
    for (;;) {
        if ((PINF & 0x70) == 0x70) {
            auto_repeat = false;
            break;
        }
        if (enable_repeat && auto_repeat)
            break;
        if (enable_repeat && i > 45) {
            auto_repeat = true;
            break;
        }
        if ((++i) >= 1000)
            return 255;
        delay(15);
    }
    // Wait for keypress
    for (;;) {
        if (BTN1_PRESSED)
            return (settings.display_rotation) ? PIN_BTN1 : PIN_BTN3;
        if (BTN2_PRESSED)
            return PIN_BTN2;
        if (BTN3_PRESSED)
            return (settings.display_rotation) ? PIN_BTN3 : PIN_BTN1;
        if ((++i) >= KBD_TIMEOUT)
            break;
        delay(15);
    }

    return 255;
}
