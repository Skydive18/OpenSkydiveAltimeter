// Battery calibration sketch
#include "EEPROM.h"
#include "Wire.h"

//#include "MPL.h"

#if defined(__AVR_ATmega32U4__)
// Pins for Arduino Pro Micro (Atmega-32u4)
#define PLATFORM_1 'A'
#define PIN_HWPWR 1
#define PIN_LIGHT 4
#define PIN_R 5
#define PIN_G 9
#define PIN_B 10
#define PIN_BTN1 A3
#define PIN_BTN2 0
#define PIN_BTN3 A1
#define PIN_BAT_SENSE A0
#define PIN_SOUND 6
#define PIN_INTERRUPT 7
#define PIN_DC 30

#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
// Pins for Arduino Pro Mini (Atmega-328[P] - based)
#define PLATFORM_1 'B'
#define PIN_HWPWR 4
#define PIN_LIGHT 7
#define PIN_R 5
#define PIN_G 6
#define PIN_B 9
#define PIN_BTN1 A3
#define PIN_BTN2 A2
#define PIN_BTN3 A1
#define PIN_BAT_SENSE A0
#define PIN_SOUND 3
#define PIN_INTERRUPT 2
#define PIN_DC 8

#else
#error CPU unsupported.
#endif // Platform

#define EEPROM_JUMP_COUNTER 0
#define EEPROM_SETTINGS 2
#define EEPROM_JUMP_PROFILES 0x20
#define EEPROM_AUDIBLE_SIGNALS 0x30

typedef struct {
    uint16_t batt_min_voltage; // Factory settings: min battery voltage, in items
    uint8_t batt_multiplier;
    uint8_t unused1;
    uint8_t unused2;
    uint8_t unused3;
    //
    uint8_t contrast : 4;
    uint8_t jump_profile_number : 4;
    // Flags byte 1
    uint8_t display_rotation : 1;
    uint8_t zero_after_reset : 1;
    uint8_t auto_power_off : 2;
    uint8_t backlight : 2;
    uint8_t use_led_signals : 1;
    uint8_t use_audible_signals : 1;
    //
    int16_t target_altitude;
    //
    uint8_t volume:2;
    uint8_t sound_amplifier_power_on:1; // this value turns sound voltage gainer / amplifier ON
    uint8_t precision_in_freefall:2; // precision in freefall: 0=no rounding, 1=10m, 2=50m, 3=100m
    uint8_t display_flipped:1; // 0 here means display is soldered in flipped position
    uint8_t sound_amplifier_power_polarity:1; // write this to PB6 to turn sound amplifier power ON
    uint8_t led_common_cathode:1; // 1 = common cathode, 0 = common anode
    //
    uint8_t volumemap[4];
    uint16_t stored_jumps;
    uint16_t stored_snapshots;
} settings_t;

typedef struct {
    uint16_t exit : 5;
    uint16_t begin_freefall : 5;
    uint16_t freefall : 6;
    uint16_t pullout : 6;
    uint16_t opening : 5;
    uint16_t under_parachute : 5;
} jump_profile_t;

typedef struct {
    uint16_t signals[8];
} audible_signals_t;

byte blinkR = 0;
byte blinkG = 0;
byte blinkB = 0;

settings_t settings;

void getBatteryAdaptives() {
    // batt_max_voltage refers to 4.18v
    analogReference(INTERNAL);
    for (byte i = 0; i < 64; ++i)
        analogRead(PIN_BAT_SENSE);
    uint16_t batt_max_voltage = analogRead(PIN_BAT_SENSE) - 3; // jitter compensate
    if (batt_max_voltage < 1020) {
        Serial.println("Use internal analog reference");
    } else {
        analogReference(DEFAULT);
        Serial.println("Use default analog reference; may be buggy");
        for (byte i = 0; i < 64; ++i)
            analogRead(PIN_BAT_SENSE);
        batt_max_voltage = analogRead(PIN_BAT_SENSE) - 3; // jitter compensate        
    }

    // Assume a value of min voltage that is 3.70v
    uint16_t batt_min_voltage = (uint16_t)((float)batt_max_voltage * (37.0f/41.8f));

    // Compute range that is max_voltage - min_voltage
    uint16_t batt_voltage_range = batt_max_voltage - batt_min_voltage;

    // Compute a cost, in %%, of one sample
    float batt_percentage_multiplier = 100.0f / ((float)batt_voltage_range);

    settings.batt_min_voltage = batt_min_voltage;
    settings.batt_multiplier = 2 + (batt_percentage_multiplier * 100);

    Serial.print("Max voltage: ");
    Serial.println(batt_max_voltage);
    Serial.print("Min voltage: ");
    Serial.println(batt_min_voltage);
    Serial.print("Range: ");
    Serial.println(batt_voltage_range);
    Serial.print("Multiplier: ");
    Serial.print(batt_percentage_multiplier);
    Serial.print("/");
    Serial.println(settings.batt_multiplier);
}

void setup() {
    Serial.begin(9600);
    // Set up pins
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_HWPWR, OUTPUT);
    pinMode(PIN_BAT_SENSE, INPUT);
    pinMode(PIN_BTN1, INPUT_PULLUP);
    pinMode(PIN_BTN2, INPUT_PULLUP);
    pinMode(PIN_BTN3, INPUT_PULLUP);
    pinMode(PIN_LIGHT, OUTPUT);
    digitalWrite(PIN_LIGHT, 0);

    pinMode(PIN_SOUND, OUTPUT);
    digitalWrite(PIN_SOUND, 0);
    
    // We need to turn hardware ON.
    // When hardware is OFF, BATT_SENSE is the same as Vref,
    // so calibration is not possible.
    digitalWrite(PIN_HWPWR, 1);
    delay(3000);
    
    EEPROM.get(EEPROM_SETTINGS, settings);

    getBatteryAdaptives();
    
    // Also initialize jump counter.
    // BEWARE! It will effectively erase the logbook.
    // Comment it out when running in existing altimeter,
    // or backup logbook before!

    //uint16_t jump_counter = 0;
    //EEPROM.put(EEPROM_JUMP_COUNTER, jump_counter);
    
    Serial.println("Scanning I2C Bus...");
    int i2cCount = 0;
    for (uint8_t i = 0x8; i < 128; i++) {
        Serial.print ("0x");
        Serial.print (i, HEX);
        Serial.print (' ');
        Wire.begin();
        Wire.beginTransmission (i);
        if (Wire.endTransmission () == 0) {
            Serial.print ("\nFound address: ");
            Serial.print (i, DEC);
            Serial.print (" (0x");
            Serial.print (i, HEX);
            Serial.println (")");
            i2cCount++;
            delay (100);  // maybe unneeded?
        } // end of good response
    } // end of for loop
    Serial.print ("\nScanning I2C Bus Done. Found ");
    Serial.print (i2cCount, DEC);
    Serial.println (" device(s).");

    byte state = 0;
    do {
        digitalWrite(PIN_G, state);
        state = (++ state) & 1;
        delay(250);
    } while (digitalRead(PIN_BTN1) && digitalRead(PIN_BTN2) && digitalRead(PIN_BTN3));

    // Write battery calibration
    getBatteryAdaptives(); // once again after cable removed
    EEPROM.put(EEPROM_SETTINGS, settings);

    // Custom hello and bye messages.
    // Available if both logbook and snapshot reside in external flash

#if 0
    uint8_t has_custom_hello_message = 0x41; // set to 0xff to disable
    char custom_hello_message[16];
    char custom_bye_message[16];
    strcpy(custom_hello_message, "Hi!");
    strcpy(custom_bye_message, "Bye");
    EEPROM.put(0x1f, has_custom_hello_message);
    EEPROM.put(0x130, custom_hello_message);
    EEPROM.put(0x140, custom_bye_message);
#else
    uint8_t has_custom_hello_message = 0xff; // set to 0xff to disable
    EEPROM.put(0x1f, has_custom_hello_message);
#endif

    // Write state machine profiles
    jump_profile_t jump_profile;

    // Profile S - Skydive
    jump_profile.exit = 7;
    jump_profile.begin_freefall = 10;
    jump_profile.freefall = 40;
    jump_profile.pullout = 40;
    jump_profile.opening = 14;
    jump_profile.under_parachute = 7;
    EEPROM.put(EEPROM_JUMP_PROFILES, jump_profile);
    
    // Profile F - Freefly
    jump_profile.exit = 7;
    jump_profile.begin_freefall = 10;
    jump_profile.freefall = 40;
    jump_profile.pullout = 40;
    jump_profile.opening = 14;
    jump_profile.under_parachute = 7;
    EEPROM.put(EEPROM_JUMP_PROFILES + sizeof(jump_profile_t), jump_profile);
    
    // Profile C - CRW, Hop&Pop
    jump_profile.exit = 7;
    jump_profile.begin_freefall = 10;
    jump_profile.freefall = 40;
    jump_profile.pullout = 40;
    jump_profile.opening = 14;
    jump_profile.under_parachute = 7;
    EEPROM.put(EEPROM_JUMP_PROFILES + (2 * sizeof(jump_profile_t)), jump_profile);
    
    // Profile W - Wingsuit
    jump_profile.exit = 7;
    jump_profile.begin_freefall = 10;
    jump_profile.freefall = 40;
    jump_profile.pullout = 40;
    jump_profile.opening = 14;
    jump_profile.under_parachute = 7;
    EEPROM.put(EEPROM_JUMP_PROFILES + (3 * sizeof(jump_profile_t)), jump_profile);

    // Write audible signals
    audible_signals_t audible_signals;

    // S1
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (0 * sizeof(audible_signals_t)), audible_signals);
    
    // S2
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (1 * sizeof(audible_signals_t)), audible_signals);

    // S3
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (2 * sizeof(audible_signals_t)), audible_signals);

    // S4
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (3 * sizeof(audible_signals_t)), audible_signals);

    // F1
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (4 * sizeof(audible_signals_t)), audible_signals);
    
    // F2
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (5 * sizeof(audible_signals_t)), audible_signals);

    // F3
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (6 * sizeof(audible_signals_t)), audible_signals);

    // F4
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (7 * sizeof(audible_signals_t)), audible_signals);

    // C1
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (8 * sizeof(audible_signals_t)), audible_signals);
    
    // C2
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (9 * sizeof(audible_signals_t)), audible_signals);

    // C3
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (10 * sizeof(audible_signals_t)), audible_signals);

    // C4
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (11 * sizeof(audible_signals_t)), audible_signals);

    // W1
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (12 * sizeof(audible_signals_t)), audible_signals);
    
    // W2
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (13 * sizeof(audible_signals_t)), audible_signals);

    // W3
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (14 * sizeof(audible_signals_t)), audible_signals);

    // W4
    audible_signals.signals[0] = 1500;
    audible_signals.signals[1] = 1500;
    audible_signals.signals[2] = 1200;
    audible_signals.signals[3] = 1000;
    audible_signals.signals[4] =  410;
    audible_signals.signals[5] =  310;
    audible_signals.signals[6] =  210;
    audible_signals.signals[7] =  110;
    EEPROM.put(EEPROM_AUDIBLE_SIGNALS + (15 * sizeof(audible_signals_t)), audible_signals);

    digitalWrite(PIN_LIGHT, 1);
}

void loop() {
    analogWrite(PIN_G, blinkG++);
    analogWrite(PIN_R, blinkR+=3);
    analogWrite(PIN_B, blinkB+=5);
    delay(50);
}
