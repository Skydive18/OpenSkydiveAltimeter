#ifndef __in_display_inc
#define __in_display_inc

#include <U8g2lib.h>
#include <U8x8lib.h>

//no U8G2_WITH_FONT_ROTATION no U8G2_WITH_INTERSECTION no U8G2_WITH_CLIP_WINDOW_SUPPORT
//If the I2C interface is not required, then uncomment #define U8X8_HAVE_HW_I2C in U8x8lib.h
#ifdef DISPLAY_NOKIA
#define DC_PIN 30
#define DISPLAY_WIDTH 84
#define DISPLAY_HEIGHT 48
U8G2_PCD8544_84X48_1_4W_HW_SPI u8g2(U8G2_R0, SS, DC_PIN, U8X8_PIN_NONE);
#else
#ifdef DISPLAY_HX1230
#define DISPLAY_WIDTH 96
#define DISPLAY_HEIGHT 68
U8G2_HX1230_96X68_1_4W_HW_SPI u8g2(U8G2_R0, SS, U8X8_PIN_NONE, U8X8_PIN_NONE);
#endif
#endif
#endif
