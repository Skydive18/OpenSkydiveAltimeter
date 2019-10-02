#ifndef __in_display_inc
#define __in_display_inc

#include <U8g2lib.h>
#include <U8x8lib.h>

#define DC_PIN 30
#define DISPLAY_WIDTH 84
#define DISPLAY_HEIGHT 48

U8G2_PCD8544_84X48_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ SS, /* dc=*/ DC_PIN, /* reset=*/ U8X8_PIN_NONE);            // Nokia 5110 Display
#endif
