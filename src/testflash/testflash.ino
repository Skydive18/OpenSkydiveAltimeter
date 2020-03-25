#include "common.h"
#include "flash.h"
#include "hwconfig.h"

extern FlashRom flashRom;

void setup() {
    Serial.begin(9600);

    pinMode(PIN_HWPWR, OUTPUT);
    digitalWrite(PIN_HWPWR, 1);

    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    pinMode(PIN_SOUND, OUTPUT);

    digitalWrite(PIN_SOUND, 0);
    
    for (;;) {
        Serial.println("Write all zeros");
        if (!checkFlash(0))
            break;

        Serial.println("Write all 0x55");
        if (!checkFlash(0x55))
            break;

        Serial.println("Write all 0xaa");
        if (!checkFlash(0xaa))
            break;

        Serial.println("Write cyclic data");
        if (!checkFlashCycle())
            break;

        Serial.println("Write all 0xff");
        if (!checkFlash(0xff))
            break;

        digitalWrite(PIN_B, 0);
        digitalWrite(PIN_G, 1);

        Serial.println("Flash looks OK.");
        break;
    }
    
}

bool checkFlash(uint8_t value) {
    uint16_t i;
    uint8_t readVal;
    for (i = 0; i < 0xffff; ++i) {
        if ((i & 0x1f) == 0) {
            digitalWrite(PIN_B, (i >> 6) & 1);
        }
        flashRom.writeByte(i, value);
    }
    Serial.println("Verifying...");
    for (i = 0; i < 0xffff; ++i) {
        if ((i & 0x1f) == 0) {
            digitalWrite(PIN_B, (i >> 6) & 1);
        }
        readVal = flashRom.readByte(i);
        if (value != readVal) {
            Serial.print(i, HEX);
            Serial.print(": expected ");
            Serial.print(value, HEX);
            Serial.print(", read ");
            Serial.print(readVal, HEX);
            digitalWrite(PIN_B, 0);
            digitalWrite(PIN_R, 1);
            return false;
        }
    }

    return true;
}

bool checkFlashCycle() {
    uint16_t i;
    uint8_t readVal;
    uint8_t value;
    for (i = 0; i < 0xffff; ++i) {
        if ((i & 0x1f) == 0) {
            digitalWrite(PIN_B, (i >> 6) & 1);
        }
        value = (uint8_t)((i & 0xff) + ((i >> 9) & 1));
        flashRom.writeByte(i, value);
    }
    Serial.println("Verifying...");
    for (i = 0; i < 0xffff; ++i) {
        if ((i & 0x1f) == 0) {
            digitalWrite(PIN_B, (i >> 6) & 1);
        }
        value = (uint8_t)((i & 0xff) + ((i >> 9) & 1));
        readVal = flashRom.readByte(i);
        if (value != readVal) {
            Serial.print(i, HEX);
            Serial.print(": expected ");
            Serial.print(value, HEX);
            Serial.print(", read ");
            Serial.print(readVal, HEX);
            digitalWrite(PIN_B, 0);
            digitalWrite(PIN_R, 1);
            return false;
        }
    }

    return true;
}

void loop() {
    
}
