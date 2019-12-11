#ifndef __in_logbook_h
#define __in_logbook_h
#include <Arduino.h>
#include "hwconfig.h"

#ifdef SNAPSHOT_ENABLE
void saveSnapshot();
#endif

#ifdef LOGBOOK_ENABLE
void saveJump();
void loadJump(uint16_t jump_number);
#endif

#endif
