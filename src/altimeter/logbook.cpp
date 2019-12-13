#include "logbook.h"
#include "custom_types.h"
#include "hwconfig.h"

#if (defined(SNAPSHOT_ENABLE) && SNAPSHOT_JOURNAL_LOCATION == LOCATION_FLASH) || (defined(LOGBOOK_ENABLE) && LOGBOOK_LOCATION == LOCATION_FLASH)
#include "Wire.h"
#include "flash.h"
FlashRom flashRom;
#endif

#if (defined(SNAPSHOT_ENABLE) && SNAPSHOT_JOURNAL_LOCATION == LOCATION_EEPROM) || (defined(LOGBOOK_ENABLE) && LOGBOOK_LOCATION == LOCATION_EEPROM)
#include <EEPROM.H>
#endif

#if defined(LOGBOOK_ENABLE) || defined(SNAPSHOT_ENABLE)
extern uint16_t total_jumps;
#endif

#ifdef LOGBOOK_ENABLE
extern jump_t current_jump;
extern settings_t settings;

void saveJump() {
    current_jump.profile = settings.jump_profile_number >> 2;
    // Calculate jump start address
    uint16_t addr = LOGBOOK_START + (total_jumps % LOGBOOK_SIZE) * sizeof(jump_t);
#if LOGBOOK_LOCATION == LOCATION_EEPROM
    EEPROM.put(addr, current_jump);
#elif LOGBOOK_LOCATION == LOCATION_FLASH
    flashRom.writeBytes(addr, sizeof(jump_t), (uint8_t*)&current_jump);
#endif
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

#ifdef SNAPSHOT_ENABLE
extern char bigbuf[SNAPSHOT_SIZE];
void saveSnapshot() {
    // Calculate snapshot start address
    uint16_t addr = SNAPSHOT_JOURNAL_START + (total_jumps % SNAPSHOT_JOURNAL_SIZE) * SNAPSHOT_SIZE;
#if SNAPSHOT_JOURNAL_LOCATION == LOCATION_EEPROM
    EEPROM.put(addr, bigbuf);
#elif SNAPSHOT_JOURNAL_LOCATION == LOCATION_FLASH
    flashRom.writeBytes(addr, SNAPSHOT_SIZE, (uint8_t*)bigbuf);
#endif
}
#endif
