/****************************************************************************
 * EEPROM 24C32 / 24C64 library for Arduino
 * Copyright (C) 2012  Julien Le Sech - www.idreammicro.com
 * version 1.0
 * date 20120218
 *
 * This file is part of the EEPROM 24C32 / 24C64 library for Arduino.
 *
 * This library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 ******************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include "flash.h"
#include "hwconfig.h"
#include "common.h"

//#define EEPROM__RD_BUFFER_SIZE    BUFFER_LENGTH
//#define EEPROM__WR_BUFFER_SIZE    (BUFFER_LENGTH - 2)
#define EEPROM__RD_BUFFER_SIZE    4
#define EEPROM__WR_BUFFER_SIZE    4

FlashRom::FlashRom() {
}

void FlashRom::writeByte(uint16_t address, uint8_t data) {
    beginTransmission(FLASH_ADDRESS);
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    Wire.write(data);
    Wire.endTransmission();
    delay(10);
}

void FlashRom::writeBytes(uint16_t address, uint16_t length, uint8_t* p_data) {
    // TODO. Use buffered read/write. Now it hangs I2C. Consider move to WsWire lib
    for (uint16_t i = 0; i < length; i++)
        writeByte(address + i, p_data[i]);
/*
    // Write first page if not aligned.
    uint8_t notAlignedLength = 0;
    uint8_t pageOffset = address % FLASH__PAGE_SIZE;
    if (pageOffset > 0) {
        notAlignedLength = FLASH__PAGE_SIZE - pageOffset;
        if (length < notAlignedLength) {
            notAlignedLength = length;
        }
        writePage(address, notAlignedLength, p_data);
        length -= notAlignedLength;
    }

    if (length > 0) {
        address += notAlignedLength;
        p_data += notAlignedLength;

        // Write complete and aligned pages.
        uint8_t pageCount = length / FLASH__PAGE_SIZE;
        for (uint8_t i = 0; i < pageCount; i++) {
            writePage(address, FLASH__PAGE_SIZE, p_data);
            address += FLASH__PAGE_SIZE;
            p_data += FLASH__PAGE_SIZE;
            length -= FLASH__PAGE_SIZE;
        }

        if (length > 0) {
            // Write remaining uncomplete page.
            writePage(address, length, p_data);
        }
    }
*/
}

uint8_t FlashRom::readByte(uint16_t address) {
    beginTransmission(FLASH_ADDRESS);
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    Wire.endTransmission(false);
    Wire.requestFrom(FLASH_ADDRESS, (uint8_t)1);
    return Wire.read();
    return 0xff;
}

void FlashRom::readBytes(uint16_t address, uint16_t length, uint8_t* p_data) {
    // TODO. Use buffered read/write. Now it hangs I2C. Consider move to WsWire lib
    for (uint16_t i = 0; i < length; i++)
        p_data[i] = readByte(address + i);
/*
    uint8_t bufferCount = length / EEPROM__RD_BUFFER_SIZE;
    for (uint8_t i = 0; i < bufferCount; i++) {
        uint16_t offset = i * EEPROM__RD_BUFFER_SIZE;
        readBuffer(address + offset, EEPROM__RD_BUFFER_SIZE, p_data + offset);
    }

    uint8_t remainingBytes = length % EEPROM__RD_BUFFER_SIZE;
    uint16_t offset = length - remainingBytes;
    readBuffer(address + offset, remainingBytes, p_data + offset);
*/    
}

#if 0
void FlashRom::writePage(uint16_t address, uint8_t length, uint8_t* p_data) {
    // Write complete buffers.
    uint8_t bufferCount = length / EEPROM__WR_BUFFER_SIZE;
    for (uint8_t i = 0; i < bufferCount; i++) {
        uint8_t offset = i * EEPROM__WR_BUFFER_SIZE;
        writeBuffer(address + offset, EEPROM__WR_BUFFER_SIZE, p_data + offset);
    }

    // Write remaining uint8_ts.
    uint8_t remainingBytes = length % EEPROM__WR_BUFFER_SIZE;
    uint8_t offset = length - remainingBytes;
    writeBuffer(address + offset, remainingBytes, p_data + offset);
}

void FlashRom::writeBuffer(uint16_t address, uint8_t length, uint8_t* p_data) {
    beginTransmission(FLASH_ADDRESS);
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    for (uint8_t i = 0; i < length; i++) {
        Wire.write(p_data[i]);
    }
    Wire.endTransmission();
        
    // Write cycle time (tWR). See EEPROM memory datasheet for more details.
    delay(10);
}

void FlashRom::readBuffer(uint16_t address, uint8_t length, uint8_t* p_data) {
    beginTransmission(FLASH_ADDRESS);
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    Wire.endTransmission(false);
    Wire.requestFrom(FLASH_ADDRESS, length);
    for (uint8_t i = 0; i < length; i++) {
        p_data[i] = Wire.read();
    }
}
#endif
