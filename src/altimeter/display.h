#ifndef __in_display_inc
#define __in_display_inc

#include <U8g2lib.h>
#include <U8x8lib.h>

//no U8G2_WITH_FONT_ROTATION no U8G2_WITH_INTERSECTION no U8G2_WITH_CLIP_WINDOW_SUPPORT
//If the I2C interface is not required, then uncomment #define U8X8_HAVE_HW_I2C in U8x8lib.h
#ifdef DISPLAY_NOKIA
#define DC_PIN PIN_DC
#define DISPLAY_WIDTH 84
#define DISPLAY_HEIGHT 48
#define DISPLAY_LINES_IN_MENU 5
U8G2_PCD8544_84X48_1_4W_HW_SPI u8g2(U8G2_R2, SS, DC_PIN, U8X8_PIN_NONE);
#else
#ifdef DISPLAY_HX1230
#define DISPLAY_WIDTH 96
#define DISPLAY_HEIGHT 68
#define DISPLAY_LINES_IN_MENU 6

// U8G2_HX1230_96X68_1_3W_SW_SPI u8g2(U8G2_R0, SS, U8X8_PIN_NONE);

// Comment it to use 3-wire hardware SPI emulation
//#define DC_PIN PIN_DC

extern "C" uint8_t u8x8_byte_arduino_hw_spi_3w(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr);

class U8G2_HX1230_96X68_1_HW_SPI : public U8G2 {
    public:
    U8G2_HX1230_96X68_1_HW_SPI(const u8g2_cb_t *rotation, uint8_t cs, uint8_t dc = U8X8_PIN_NONE, uint8_t reset = U8X8_PIN_NONE) : U8G2() {
#ifdef DC_PIN        
        u8g2_Setup_hx1230_96x68_1(&u8g2, rotation, u8x8_byte_arduino_hw_spi, u8x8_gpio_and_delay_arduino);
#else
        u8g2_Setup_hx1230_96x68_1(&u8g2, rotation, u8x8_byte_arduino_hw_spi_3w, u8x8_gpio_and_delay_arduino);
#endif
        u8x8_SetPin_4Wire_HW_SPI(getU8x8(), cs, dc, reset);
    }
};

U8G2_HX1230_96X68_1_HW_SPI u8g2(U8G2_R0, SS, PIN_DC);

#endif
#endif
#endif
