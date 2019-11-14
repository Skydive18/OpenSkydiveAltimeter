/*
    Contains parts of code from

*   ucglib = universal color graphics library
    ucglib = micro controller graphics library
    Universal uC Color Graphics Library
    Copyright (c) 2014, olikraus@gmail.com

*   Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
    Copyright (c) 2016, olikraus@gmail.com

*/
#include "U8x8lib.h"
#include <SPI.h>

static uint8_t arduino_hw_spi_3w_buffer[9];
static uint8_t arduino_hw_spi_3w_bytepos;
static uint16_t arduino_hw_spi_3w_dc; // 0 = dc==0, 256 = dc==1

static void arduino_hw_spi_3w_init() {
    memset(arduino_hw_spi_3w_buffer, 0, 9);
    arduino_hw_spi_3w_bytepos = 0;
}

static void arduino_hw_spi_3w_flush() {
    for(uint8_t i = 0; i <= arduino_hw_spi_3w_bytepos; i++) {
        SPI.transfer(arduino_hw_spi_3w_buffer[i]);
    }
}

static void arduino_hw_spi_3w_sendbyte(uint8_t data) {
    static union { uint16_t val; struct { uint8_t lsb; uint8_t msb; }; } data16;
    data16.val = (arduino_hw_spi_3w_dc + data) << (7 - arduino_hw_spi_3w_bytepos);
    // AVR stores least significant byte first
    arduino_hw_spi_3w_buffer[arduino_hw_spi_3w_bytepos]   |= data16.msb;
    arduino_hw_spi_3w_buffer[++arduino_hw_spi_3w_bytepos] |= data16.lsb;
    if (arduino_hw_spi_3w_bytepos == 8) {
        arduino_hw_spi_3w_flush();
        arduino_hw_spi_3w_init();
    }
}

extern "C" uint8_t u8x8_byte_arduino_hw_spi_3w(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr) {

    uint8_t *data;
    uint8_t internal_spi_mode;

    switch(msg) {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while(arg_int > 0) {
                arduino_hw_spi_3w_sendbyte((uint8_t)*data);
                data++;
                arg_int--;
            }
        break;

        case U8X8_MSG_BYTE_INIT:
            if (u8x8->bus_clock == 0)
                u8x8->bus_clock = u8x8->display_info->sck_clock_hz;
            /* disable chipselect */
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            SPI.begin();
        break;
      
        case U8X8_MSG_BYTE_SET_DC:
            arduino_hw_spi_3w_dc = arg_int ? 256 : 0;
        break;
      
        case U8X8_MSG_BYTE_START_TRANSFER:
            /* SPI mode has to be mapped to the mode of the current controller;
               at least Uno, Due, 101 have different SPI_MODEx values */
            internal_spi_mode =  0;
            switch(u8x8->display_info->spi_mode) {
                case 0: internal_spi_mode = SPI_MODE0; break;
                case 1: internal_spi_mode = SPI_MODE1; break;
                case 2: internal_spi_mode = SPI_MODE2; break;
                case 3: internal_spi_mode = SPI_MODE3; break;
            }
      
#if ARDUINO >= 10600
            SPI.beginTransaction(
                SPISettings(u8x8->bus_clock, MSBFIRST, internal_spi_mode));
#else
            SPI.begin();
            if (u8x8->display_info->sck_pulse_width_ns < 70)
                SPI.setClockDivider(SPI_CLOCK_DIV2);
            else if (u8x8->display_info->sck_pulse_width_ns < 140)
                SPI.setClockDivider(SPI_CLOCK_DIV4);
            else
                SPI.setClockDivider(SPI_CLOCK_DIV8);
            SPI.setDataMode(internal_spi_mode);
            SPI.setBitOrder(MSBFIRST);
#endif
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
            u8x8->gpio_and_delay_cb(
                u8x8,
                U8X8_MSG_DELAY_NANO,
                u8x8->display_info->post_chip_enable_wait_ns,
                NULL);
            arduino_hw_spi_3w_init();
        break;

        case U8X8_MSG_BYTE_END_TRANSFER:      
            u8x8->gpio_and_delay_cb(
                u8x8,
                U8X8_MSG_DELAY_NANO,
                u8x8->display_info->pre_chip_disable_wait_ns,
                NULL);
            if(arduino_hw_spi_3w_bytepos)
                arduino_hw_spi_3w_flush();
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);

#if ARDUINO >= 10600
            SPI.endTransaction();
#else
            SPI.end();
#endif
        break;

        default:
            return 0;
    }
  
    return 1;
}
