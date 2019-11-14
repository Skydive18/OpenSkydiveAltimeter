#ifndef __in_logbook_h
#define __in_logbook_h
#include <Arduino.h>
#include "hwconfig.h"
#if SNAPSHOT_JOURNAL_LOCATION == RAM
#include "PCF8583.h"
#endif
#if SNAPSHOT_JOURNAL_LOCATION == FLASH || LOGBOOK_LOCATION == FLASH
#include "Wire.h"
#endif

#endif
