#ifndef __in_common_h
#define __in_common_h

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
    uint16_t batt_min_voltage; // Factory settings: min battery voltage, in items
    uint8_t batt_multiplier;
    uint8_t unused1;
    uint8_t unused2;
    uint8_t unused3;
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

void beginTransmission(uint8_t iicAddr);
uint8_t IIC_ReadByte(uint8_t iicAddr, uint8_t regAddr);
void IIC_WriteByte(uint8_t iicAddr, uint8_t regAddr, uint8_t value);
void IIC_WriteByte(uint8_t iicAddr, uint8_t value);

int IIC_ReadInt(uint8_t iicAddr, uint8_t regAddr);
void IIC_WriteInt(uint8_t iicAddr,uint8_t regAddr, int value);

long ByteToHeartbeat(uint8_t hbAsByte);
uint8_t HeartbeatValue(uint8_t hbAsByte);
int precisionMultiplier();

void hardwareReset();
void termSerial();
#endif
