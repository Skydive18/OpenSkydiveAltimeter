#include <Arduino.h>
#include <EEPROM.h>
#include "hwconfig.h"
#include "common.h"
#include "MPL.h"
#include "RTC.h"
#include "logbook.h"
#include "automatics.h"

//
// State machine points
//
#define EXIT_TH -jump_profile.exit
#define BEGIN_FREEFALL_TH -jump_profile.begin_freefall
#define FREEFALL_TH -jump_profile.freefall
#define PULLOUT_TH -jump_profile.pullout
#define OPENING_TH -jump_profile.opening
#define UNDER_PARACHUTE_TH -jump_profile.under_parachute

extern uint8_t altimeter_mode;
extern uint8_t previous_altimeter_mode;
extern int current_altitude; // currently measured altitude, relative to ground_altitude
extern int previous_altitude; // Previously measured altitude, relative to ground_altitude
extern int altitude_to_show; // Last shown altitude (jitter corrected)
extern jump_profile_t jump_profile;
extern settings_t settings;
extern int vspeed[32]; // used in mode detector. TODO clean it in setup maybe?
extern uint8_t vspeedPtr; // pointer to write to vspeed buffer
extern int current_speed;
extern int average_speed_8;
extern int average_speed_32;

#ifdef LOGBOOK_ENABLE
extern jump_t current_jump;
#endif

#if defined(LOGBOOK_ENABLE) || defined(SNAPSHOT_ENABLE)
extern uint16_t total_jumps;
#endif

extern char bigbuf[SNAPSHOT_SIZE];

// Hardware
extern MPL3115A2 myPressure;
extern Rtc rtc;

// Auto zero drift correction
uint8_t zero_drift_sense = 128;
uint16_t accumulated_altitude;
int accumulated_speed;

void saveToJumpSnapshot() {
    if (current_jump.total_jump_time < 2045)
        current_jump.total_jump_time++;

    if (current_jump.total_jump_time & 1)
        return; // Do computations once per second

    int speed_now = current_altitude - accumulated_altitude;
    // Limit acceleration (in fact, this is exactly what we store) with 1.1g down, 2,5g up.
    // Anyway, while freefall, acceleration more than 1g down is not possible.
    int8_t accel = (speed_now - accumulated_speed);
    if (accel < -11)
        accel = -11;
    if (accel > 25)
        accel = 25;
#ifdef SNAPSHOT_ENABLE
    if (current_jump.total_jump_time < (SNAPSHOT_SIZE * 2))
        bigbuf[current_jump.total_jump_time >> 1] = (char)(speed_now - accumulated_speed);
#endif
    accumulated_speed += accel;
    accumulated_altitude += accumulated_speed;
}


//
// Main state machine
// Ideas got from EZ430 project at skycentre.net
//
void processAltitude() {
    if (previous_altitude == CLEAR_PREVIOUS_ALTITUDE) { // unknown delay or first run; just save
        previous_altitude = current_altitude;
        return;
    }
    
    current_speed = (current_altitude - previous_altitude) << 1; // ok there; sign bit will be kept because <int> is big enough

    vspeed[vspeedPtr & 31] = (int8_t)current_speed;
    // Calculate averages
    uint8_t startPtr = ((uint8_t)(vspeedPtr)) & 31;
    average_speed_32 = 0;
    for (uint8_t i = 0; i < 32; i++) {
        average_speed_32 += vspeed[(startPtr - i) & 31];
        if (i == 7)
            average_speed_8 = average_speed_32 / 8; // sign extension used
    }
    average_speed_32 /= 32;
    vspeedPtr++;

    int jitter = current_altitude - altitude_to_show;

    // Compute altitude to show, compensate drift
    if (altimeter_mode == MODE_ON_EARTH) {
        // Jitter compensation
        if (jitter < 5 && jitter > -5) {
            // Inside jitter range. Try to correct zero drift.
            if (altitude_to_show == 0) {
                zero_drift_sense += jitter;

                if (zero_drift_sense < 8) {
                    // Do correct zero drift
                    myPressure.zero();
                    // Altitude already shown as 0. current_altitude is not needed anymore.
                    // It will be re-read in next cycle and assumed to be zero, as we corrected
                    // zero drift.
                    zero_drift_sense = 128;
                }
            } else {
                // Not at ground
                zero_drift_sense = 128;
            }
        } else {
            // jitter out of range
            altitude_to_show = current_altitude;
            zero_drift_sense = 128;
        }
    } else {
        // Do not perform zero drift compensation and jitter correction in modes other than MODE_ON_EARTH
        if (altimeter_mode > MODE_IN_AIRPLANE && altimeter_mode < MODE_OPENING) {
            // Round altitude in freefall
            //altitude_to_show = int(((float)current_altitude / precisionMultiplier()) + 0.5) * precisionMultiplier();
            altitude_to_show = (current_altitude / precisionMultiplier()) * precisionMultiplier();
        } else
            altitude_to_show = current_altitude;
        zero_drift_sense = 128;
    }

    if (altimeter_mode == MODE_PREFILL) {
        // Change mode to ON_EARTH if vspeed has been populated (approx. 16s after enter to this mode)
        if (vspeedPtr > 32) {
            // set to zero if we close to it
            if (settings.zero_after_reset && current_altitude < 450) {
                altitude_to_show = 0;
                myPressure.zero();
            }
            altimeter_mode = MODE_ON_EARTH;
        }
    } else

    //
    // ************************
    // ** MAIN STATE MACHINE **
    // ************************
    //
    // Due to compiler optimization, **DO NOT** use `case` here.
    // Also, **DO NOT** remove `else`'s.

    if (altimeter_mode == MODE_ON_EARTH) {
        if (current_altitude > 30 && average_speed_8 > 1)
            altimeter_mode = MODE_IN_AIRPLANE;
        else if (average_speed_8 <= FREEFALL_TH) {
            altimeter_mode = MODE_FREEFALL;
#ifdef LOGBOOK_ENABLE
            current_jump.total_jump_time = 2047; // do not store this jump in logbook
#endif
        }
    } else
    
    if (altimeter_mode == MODE_IN_AIRPLANE) {
        if (current_speed <= EXIT_TH) {
            altimeter_mode = MODE_EXIT;
#ifdef LOGBOOK_ENABLE
            current_jump.exit_altitude = (current_altitude) >> 1;
            current_jump.exit_timestamp = rtc.getTimestamp();
            current_jump.total_jump_time = current_jump.max_freefall_speed_ms = 0;
#endif
#ifdef SNAPSHOT_ENABLE
            bigbuf[0] = (char)current_speed;
#endif
            accumulated_altitude = current_altitude;
            accumulated_speed = current_speed;
        }
        else if (average_speed_32 < 1 && current_altitude < 25)
            altimeter_mode = MODE_ON_EARTH;
    } else
    
    if (altimeter_mode == MODE_EXIT) {
        saveToJumpSnapshot();
        if (accumulated_speed <= BEGIN_FREEFALL_TH) {
            altimeter_mode = MODE_BEGIN_FREEFALL;
        }
        else if (accumulated_speed > EXIT_TH)
            altimeter_mode = MODE_IN_AIRPLANE;
    } else
    
    if (altimeter_mode == MODE_BEGIN_FREEFALL) {
        saveToJumpSnapshot();
        if (accumulated_speed <= FREEFALL_TH)
            altimeter_mode = MODE_FREEFALL;
        else if (accumulated_speed > BEGIN_FREEFALL_TH)
            altimeter_mode = MODE_EXIT;
    } else
    
    if (altimeter_mode == MODE_FREEFALL) {
        saveToJumpSnapshot();
        if (accumulated_speed > PULLOUT_TH)
            altimeter_mode = MODE_PULLOUT;
#ifdef LOGBOOK_ENABLE
        current_jump.deploy_altitude = current_altitude >> 1;
        current_jump.deploy_time = current_jump.total_jump_time;
        int8_t freefall_speed = 0 - average_speed_8;
        if (freefall_speed > current_jump.max_freefall_speed_ms)
            current_jump.max_freefall_speed_ms = freefall_speed;
#endif
    } else
    
    if (altimeter_mode == MODE_PULLOUT) {
        saveToJumpSnapshot();
        if (accumulated_speed <= PULLOUT_TH)
            altimeter_mode = MODE_FREEFALL;
        else if (accumulated_speed > OPENING_TH) {
            altimeter_mode = MODE_OPENING;
        }
    } else
    
    if (altimeter_mode == MODE_OPENING) {
        saveToJumpSnapshot();
        if (accumulated_speed <= OPENING_TH)
            altimeter_mode = MODE_PULLOUT;
        else if (average_speed_8 > UNDER_PARACHUTE_TH) {
            altimeter_mode = MODE_UNDER_PARACHUTE;
#ifdef LOGBOOK_ENABLE
            current_jump.canopy_altitude = current_jump.deploy_altitude - (current_altitude >> 1);
#endif
        }
    } else
    
    if (altimeter_mode == MODE_UNDER_PARACHUTE) {
        saveToJumpSnapshot();
        if (average_speed_32 == 0) {
            altimeter_mode = MODE_ON_EARTH;
            // try to lock zero altitude
            altitude_to_show = 0;
#ifdef LOGBOOK_ENABLE
            saveJump();
#endif
        }
    }
    
}
