#ifndef __in_display_h
#define __in_display_h

#include <U8g2lib.h>
#include <U8x8lib.h>

//no U8G2_WITH_FONT_ROTATION no U8G2_WITH_INTERSECTION no U8G2_WITH_CLIP_WINDOW_SUPPORT
//If the I2C interface is not required, then uncomment #define U8X8_HAVE_HW_I2C in U8x8lib.h
#if DISPLAY==DISPLAY_NOKIA5110
#define DC_PIN PIN_DC
#define DISPLAY_WIDTH 84
#define DISPLAY_HEIGHT 48
#define DISPLAY_LINES_IN_MENU 5
#define DISPLAY_LIGHT_OFF PORTD |= 0x80
#define DISPLAY_LIGHT_ON PORTD &= 0x7f

U8G2_PCD8544_84X48_2_4W_HW_SPI u8g2(U8G2_R2, SS, DC_PIN, U8X8_PIN_NONE);

#elif DISPLAY==DISPLAY_NOKIA1201

#define DISPLAY_WIDTH 96
#define DISPLAY_HEIGHT 68
#define DISPLAY_LINES_IN_MENU 8
#define DISPLAY_LIGHT_OFF PORTD |= 0x80
#define DISPLAY_LIGHT_ON PORTD &= 0x7f

// 3-wire software SPI
//U8G2_HX1230_96X68_2_3W_SW_SPI u8g2(U8G2_R0, SCK, MOSI, SS, PIN_DC);
// 3-wire over-hardware SPI
U8G2_HX1230_96X68_2_3W_HW_SPI u8g2(U8G2_R0, SS, PIN_DC);
#endif

#endif
