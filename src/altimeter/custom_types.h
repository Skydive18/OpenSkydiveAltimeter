#ifndef __in_custom_types_h
#define __in_custom_types_h

typedef struct {
    uint32_t year   : 6;   // 0-63, base year is 2016, should be leap year
    uint32_t month  : 4;   // 1..12 [4 bits]
    uint32_t date   : 5;   // 1..31 [5 bits]
    uint32_t hour   : 5;   // 00..23 [5 bits]
    uint32_t minute : 6;   // 00..59 [6 bits]
    uint32_t second : 6;   // 00..59 [6 bits]  
} Timestamp;

typedef struct {
    Timestamp timestamp;
    // Jump data
    uint32_t exitAltitude : 11; // Exit altitude / 4, meters (0..8191)
    uint32_t deployAltitude : 11; // Deploy altitude / 4, meters (0..8191)
    uint32_t freefallTime : 10; // Freefall time, seconds (0..512)
    unsigned short maxFreefallSpeedMS; // Max freefall speed, m/s
} JUMP; // structure size: 9 bytes

typedef struct {
    byte blue_th;
    byte green_th;
    byte yellow_th;
    byte red_th;
} LED_PROFILE;

typedef struct {
    float battGranulationF; // Factory settings: battery percentage per 1 digitalRead item
    unsigned int battGranulationD; // Factory settings: min battery voltage, in items
    unsigned short jumpCount; // 0-100
    unsigned short nextJumpPtr; // Pointer to next stored jump if count == 100
} SETTINGS;

#endif
