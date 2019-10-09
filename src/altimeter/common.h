#ifndef __in_common_h
#define __in_common_h

uint8_t IIC_ReadByte(uint8_t iicAddr, uint8_t regAddr);
void IIC_WriteByte(uint8_t iicAddr, uint8_t regAddr, uint8_t value);

int IIC_ReadInt(uint8_t iicAddr, uint8_t regAddr);
void IIC_WriteInt(uint8_t iicAddr,uint8_t regAddr, int value);

#endif
