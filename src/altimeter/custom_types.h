#ifndef __in_custom_types_h
#define __in_custom_types_h

typedef struct {
    uint32_t year   : 12;  //
    uint32_t month  : 4;   // 1..12 [4 bits]
    uint32_t day    : 5;   // 1..31 [5 bits]
    uint32_t hour   : 5;   // 00..23 [5 bits]
    uint32_t minute : 6;   // 00..59 [6 bits]
} timestamp_t;

typedef struct {
    timestamp_t exit_timestamp;
    uint32_t exit_altitude : 12; // *2, 0..8191
    uint32_t deploy_altitude : 12; // *2, 0..8191
    uint32_t max_freefall_speed_ms : 8; // 
    //
    uint32_t canopy_altitude : 10; // *2, 0..2048, delta from deploy_altitude
    uint32_t deploy_time : 10; // in 1s ticks after exit, 512 seconds max
    uint32_t total_jump_time : 12; // in 500ms ticks, 1024s max
} jump_t;

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
