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
    uint32_t deploy_time : 9; // in 1s ticks after exit, 512 seconds max
    uint32_t total_jump_time : 11; // in 500ms ticks, 1024s max
    uint32_t profile : 2; // Jump profile, 0..3
} jump_t;

typedef struct {
    uint16_t battGranulationD; // Factory settings: min battery voltage, in items
    float battGranulationF; // Factory settings: battery percentage per 1 digitalRead item
    uint8_t contrast : 4;
    uint8_t jump_profile_number : 4;
    // Flags
    uint8_t display_rotation : 1;
    uint8_t zero_after_reset : 1;
    uint8_t auto_power_off : 2;
    uint8_t backlight : 2;
    uint8_t use_led_signals : 1;
    uint8_t use_audible_signals : 1;
    //
    int16_t target_altitude;
    uint8_t volume:2;
    uint8_t alarm_volume:2;
    uint8_t volumemap[4];
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

#endif
