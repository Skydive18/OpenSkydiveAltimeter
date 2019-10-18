// Battery calibration sketch
#include "EEPROM.h"

#if defined(__AVR_ATmega32U4__)
// Pins for Arduino Pro Micro (Atmega-32u4)
#define PIN_HWPWR 8
#define PIN_G 9
#define PIN_BAT_SENSE A0
#else
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
// Pins for Arduino Pro Mini (Atmega-328[P] - based)
#define PIN_HWPWR 4
#define PIN_G 6
#define PIN_BAT_SENSE A0
#endif // defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#endif // defined(__AVR_ATmega32U4__)

#define EEPROM_JUMP_COUNTER 0
#define EEPROM_SETTINGS 2

typedef struct {
    uint16_t battGranulationD; // Factory settings: min battery voltage, in items
    float battGranulationF; // Factory settings: battery percentage per 1 digitalRead item
} settings_t;

byte blinkbyte = 0;

void setup() {
    Serial.begin(57600);
    // Set up pins
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_HWPWR, OUTPUT);
    pinMode(PIN_BAT_SENSE, INPUT);

    // We need to turn hardware ON.
    // When hardware is OFF, BATT_SENSE is the same as Vref,
    // so calibration is not possible.
    digitalWrite(PIN_HWPWR, 1);
    delay(2000);
    
    // batt_max_voltage refers to 4.20v
    uint16_t batt_max_voltage = analogRead(PIN_BAT_SENSE);

    // Assume a value of min voltage that is 3.60v
    uint16_t batt_min_voltage = batt_max_voltage * (36.0f/42.0f);

    // Compute range that is max_voltage - min_voltage
    uint16_t batt_voltage_range = batt_max_voltage - batt_min_voltage;

    // Compute a cost, in %%, of one sample
    float batt_percentage_multiplier = ((float)batt_voltage_range) / 100.0f;

    // Save settings to nvram
    settings_t settings;
    settings.battGranulationD = batt_min_voltage;
    settings.battGranulationF = batt_percentage_multiplier;
    EEPROM.put(EEPROM_SETTINGS, settings);

    // Also initialize jump counter.
    // BEWARE! It will effectively erase the logbook.
    // Comment it out when running in existing altimeter,
    // or backup logbook before!

    uint16_t jump_counter = 0;
    //EEPROM.put(EEPROM_JUMP_COUNTER, jump_counter);
    
    Serial.print("Max voltage: ");
    Serial.println(batt_max_voltage);
    Serial.print("Min voltage: ");
    Serial.println(batt_min_voltage);
    Serial.print("Range: ");
    Serial.println(batt_voltage_range);
    Serial.print("Multiplier: ");
    Serial.println(batt_percentage_multiplier);
}

void loop() {
    analogWrite(PIN_G, blinkbyte++);
    delay(50);
}
