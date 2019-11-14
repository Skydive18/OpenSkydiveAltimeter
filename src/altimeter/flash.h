/****************************************************************************
 * EEPROM library for Arduino
 * Copyright (C) 2012  Julien Le Sech - www.idreammicro.com
 * version 1.0
 * date 20120203
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

#ifndef __in_flash_h
#define __in_flash_h

#include <Arduino.h>

class FlashRom {
    public:
        FlashRom(uint8_t deviceAddress);
        void initialize();
        void writeByte(uint16_t address, uint8_t data);
        void writeBytes(uint16_t address, uint16_t length, uint8_t* p_data);
        uint8_t readByte(uint16_t address);
        void readBytes(uint16_t address, uint16_t length, uint8_t* p_buffer);
    private:
        uint8_t m_deviceAddress;
        void writePage(uint16_t address, uint8_t length, uint8_t* p_data);
        void writeBuffer(uint16_t address, uint8_t length, uint8_t* p_data);
        void readBuffer(uint16_t address, uint8_t length, uint8_t* p_data);
};

#endif
