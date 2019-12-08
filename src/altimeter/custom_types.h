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
    byte display_rotation;
    byte zero_after_reset;
    byte contrast;
    byte auto_power_off;
    byte backlight;
    int ground_altitude;
    byte jump_profile_number;
    int target_altitude;
} settings_t;

typedef struct {
    uint32_t exit : 5;
    uint32_t begin_freefall : 5;
    uint32_t freefall : 6;
    uint32_t pullout : 6;
    uint32_t opening : 5;
    uint32_t under_parachute : 5;
} jump_profile_t;

#endif
