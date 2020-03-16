#include <Arduino.h>
#include <Wire.h>
#include "common.h"

void beginTransmission(uint8_t iicAddr) {
    Wire.begin();
    Wire.beginTransmission(iicAddr);
}

uint8_t IIC_ReadByte(uint8_t iicAddr, uint8_t regAddr) {
    beginTransmission(iicAddr);
    Wire.write(regAddr);
    Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
    Wire.requestFrom(iicAddr, (uint8_t)1); // Request the data...
    return Wire.read();
}

void IIC_WriteByte(uint8_t iicAddr, uint8_t regAddr, uint8_t value) {
    beginTransmission(iicAddr);
    Wire.write(regAddr);
    Wire.write(value);
    Wire.endTransmission(true);    
}

void IIC_WriteByte(uint8_t iicAddr, uint8_t value) {
    beginTransmission(iicAddr);
    Wire.write(value);
    Wire.endTransmission(true);    
}

int IIC_ReadInt(uint8_t iicAddr, uint8_t regAddr) {
    beginTransmission(iicAddr);
    Wire.write(regAddr);
    Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
    Wire.requestFrom(iicAddr, (uint8_t)2); // Request the data...
    return (Wire.read() | (Wire.read() << 8));
}


void IIC_WriteInt(uint8_t iicAddr, uint8_t regAddr, int value) {
    beginTransmission(iicAddr);
    Wire.write(regAddr);
    Wire.write(value);
    Wire.write(value >> 8);
    Wire.endTransmission(true);    
}

long ByteToHeartbeat(uint8_t hbAsByte) {
    return 7200L * HeartbeatValue(hbAsByte);
}

uint8_t HeartbeatValue(uint8_t hbAsByte) {
    switch(hbAsByte) {
        case 1: return 14; // 14hrs
        case 2: return 12; // 12 hrs
        case 3: return 10; // 10 hrs
        default: return 18;
    }
}

void(* resetFunc) (void) = 0;

void hardwareReset() {
    DDRB |= 0x80;
    PORTB &= 0x7f;
    resetFunc();
}
