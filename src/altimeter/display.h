#ifndef __in_display_h
#define __in_display_h

#include <U8g2lib.h>
#include <U8x8lib.h>

//no U8G2_WITH_FONT_ROTATION no U8G2_WITH_INTERSECTION no U8G2_WITH_CLIP_WINDOW_SUPPORT
//If the I2C interface is not required, then uncomment #define U8X8_HAVE_HW_I2C in U8x8lib.h
#ifdef DISPLAY_NOKIA
#define DC_PIN PIN_DC
#define DISPLAY_WIDTH 84
#define DISPLAY_HEIGHT 48
#define DISPLAY_LINES_IN_MENU 5
#define DISPLAY_LIGHT_OFF digitalWrite(PIN_LIGHT,1)
#define DISPLAY_LIGHT_ON digitalWrite(PIN_LIGHT,0)
U8G2_PCD8544_84X48_2_4W_HW_SPI u8g2(U8G2_R2, SS, DC_PIN, U8X8_PIN_NONE);

#else
#ifdef DISPLAY_HX1230

#define DISPLAY_WIDTH 96
#define DISPLAY_HEIGHT 68
#define DISPLAY_LINES_IN_MENU 8
#define DISPLAY_LIGHT_OFF digitalWrite(PIN_LIGHT,0)
#define DISPLAY_LIGHT_ON digitalWrite(PIN_LIGHT,1)

extern "C" uint8_t u8x8_byte_arduino_hw_spi_3w(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t *u8g2_m_12_9_4(uint8_t *page_cnt) {
    static uint8_t buf[384];
    *page_cnt = 4;
    return buf;
}

void u8g2_Setup_hx1230_96x68_4(u8g2_t *u8g2, const u8g2_cb_t *rotation, u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb) {
    uint8_t tile_buf_height;
    uint8_t *buf;
    u8g2_SetupDisplay(u8g2, u8x8_d_hx1230_96x68, u8x8_cad_001, byte_cb, gpio_and_delay_cb);
    buf = u8g2_m_12_9_4(&tile_buf_height);
    u8g2_SetupBuffer(u8g2, buf, tile_buf_height, u8g2_ll_hvline_vertical_top_lsb, rotation);
}

class U8G2_HX1230_96X68_X_HW_SPI : public U8G2 {
    public:
    U8G2_HX1230_96X68_X_HW_SPI(const u8g2_cb_t *rotation, uint8_t cs, uint8_t reset = U8X8_PIN_NONE) : U8G2() {
//        u8g2_Setup_hx1230_96x68_1(&u8g2, rotation, u8x8_byte_arduino_hw_spi_3w, u8x8_gpio_and_delay_arduino);
        u8g2_Setup_hx1230_96x68_2(&u8g2, rotation, u8x8_byte_arduino_hw_spi_3w, u8x8_gpio_and_delay_arduino);
//        u8g2_Setup_hx1230_96x68_4(&u8g2, rotation, u8x8_byte_arduino_hw_spi_3w, u8x8_gpio_and_delay_arduino);
//        u8g2_Setup_hx1230_96x68_F(&u8g2, rotation, u8x8_byte_arduino_hw_spi_3w, u8x8_gpio_and_delay_arduino);
        u8x8_SetPin_4Wire_HW_SPI(getU8x8(), cs, U8X8_PIN_NONE, reset);
    }
};

// 3-wire software SPI
U8G2_HX1230_96X68_2_3W_SW_SPI u8g2(U8G2_R0, SCK, MOSI, SS, U8X8_PIN_NONE);
// 3-wire over-hardware SPI
//U8G2_HX1230_96X68_X_HW_SPI u8g2(U8G2_R0, SS, U8X8_PIN_NONE);

#endif
#endif
#endif
