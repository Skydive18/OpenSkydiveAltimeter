#include "logbook.h"
#include "common.h"
#include "hwconfig.h"

#if (defined(SNAPSHOT_ENABLE) && SNAPSHOT_JOURNAL_LOCATION == LOCATION_FLASH) || (defined(LOGBOOK_ENABLE) && LOGBOOK_LOCATION == LOCATION_FLASH)
#include "flash.h"
FlashRom flashRom;
#endif

#include <EEPROM.H>

#ifdef LOGBOOK_ENABLE
extern jump_t current_jump;
extern settings_t settings;

#ifdef SNAPSHOT_ENABLE
extern char bigbuf[SNAPSHOT_SIZE];
void saveSnapshot() {
    // Calculate snapshot start address
    uint16_t addr = SNAPSHOT_JOURNAL_START + (settings.total_jumps % SNAPSHOT_JOURNAL_SIZE) * SNAPSHOT_SIZE;
#if SNAPSHOT_JOURNAL_LOCATION == LOCATION_EEPROM
    EEPROM.put(addr, bigbuf);
#elif SNAPSHOT_JOURNAL_LOCATION == LOCATION_FLASH
    flashRom.writeBytes(addr, SNAPSHOT_SIZE, (uint8_t*)bigbuf);
#endif
    if (settings.stored_snapshots < SNAPSHOT_JOURNAL_SIZE)
        settings.stored_snapshots++;
}
#endif

void saveJump() {
#ifdef SNAPSHOT_ENABLE
    saveSnapshot();
#endif
    current_jump.profile = settings.jump_profile_number >> 2;
    // Calculate jump start address
    uint16_t addr = LOGBOOK_START + (settings.total_jumps % LOGBOOK_SIZE) * sizeof(jump_t);
#if LOGBOOK_LOCATION == LOCATION_EEPROM
    EEPROM.put(addr, current_jump);
#elif LOGBOOK_LOCATION == LOCATION_FLASH
    flashRom.writeBytes(addr, sizeof(jump_t), (uint8_t*)&current_jump);
#endif
    settings.total_jumps++;
    if (settings.stored_jumps < LOGBOOK_SIZE)
        settings.stored_jumps++;
    EEPROM.put(EEPROM_SETTINGS, settings);
}

void loadJump(uint16_t jump_number) {
    // Calculate jump start address
    uint16_t addr = LOGBOOK_START + (jump_number % LOGBOOK_SIZE) * sizeof(jump_t);
#if LOGBOOK_LOCATION == LOCATION_EEPROM
    EEPROM.get(addr, current_jump);
#elif LOGBOOK_LOCATION == LOCATION_FLASH
    flashRom.readBytes(addr, sizeof(jump_t), (uint8_t*)&current_jump);
#endif
}

#endif
