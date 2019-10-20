/*
 MPL3115A2 Barometric Pressure Sensor Library
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 22nd, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 This library allows an Arduino to read from the MPL3115A2 low-cost high-precision pressure sensor.
 
 If you have feature suggestions or need support please use the github support page: https://github.com/sparkfun/MPL3115A2_Breakout

 Hardware Setup: The MPL3115A2 lives on the I2C bus. Attach the SDA pin to A4, SCL to A5. Use inline 10k resistors
 if you have a 5V board. If you are using the SparkFun breakout board you *do not* need 4.7k pull-up resistors 
 on the bus (they are built-in).
 
 Link to the breakout board product:
 
 Software:
 .begin() Gets sensor on the I2C bus.
 .readAltitude() Returns float with meters above sealevel. Ex: 1638.94
 .setOversampleRate(byte) Sets the # of samples from 1 to 128. See datasheet.
 .enableEventFlags() Sets the fundamental event flags. Required during setup.
 
 */

#include <Wire.h>

#include "osaMPL3115A2.h"
#include "common.h"

MPL3115A2::MPL3115A2() {
  //Set initial values for private vars
}

//Begin
/*******************************************************************************************/
//Start I2C communication
void MPL3115A2::begin(void) {
    Wire.begin();
    IIC_WriteByte(MPL3115A2_ADDRESS, CTRL_REG1, (IIC_ReadByte(MPL3115A2_ADDRESS, CTRL_REG1) & B11000111) | B10101000);
    IIC_WriteByte(MPL3115A2_ADDRESS, PT_DATA_CFG, 0x07);  
}


//Returns the number of meters above sea level
//Returns -999 if no new data is available
int MPL3115A2::readAltitude()
{
    toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

    //Wait for PDR bit, indicates we have new pressure data
    int counter = 0;
    while( (IIC_ReadByte(MPL3115A2_ADDRESS, STATUS) & (1<<1)) == 0) {
        if(++counter > 600) return(-998); //Error out after max of 512ms for a read
        delay(1);
    }

    // Read pressure registers
    Wire.beginTransmission(MPL3115A2_ADDRESS);
    Wire.write(OUT_P_MSB);  // Address of data to get
    Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
    if (Wire.requestFrom(MPL3115A2_ADDRESS, 3) != 3) { // Request three bytes
        return -999;
    }

    byte msb, csb, lsb;
    msb = Wire.read();
    csb = Wire.read();
    lsb = Wire.read();

    // The least significant bytes l_altitude and l_temp are 4-bit,
    // fractional values, so you must cast the calulation in (float),
    // shift the value over 4 spots to the right and divide by 16 (since 
    // there are 16 values in 4-bits). 

    return ((msb << 8) | csb); // + (lsb >> 7));
}

//Clears then sets the OST bit which causes the sensor to immediately take another reading
//Needed to sample faster than 1Hz
void MPL3115A2::toggleOneShot(void)
{
  byte tempSetting = IIC_ReadByte(MPL3115A2_ADDRESS, CTRL_REG1); //Read current settings
  tempSetting &= ~(1<<1); //Clear OST bit
  IIC_WriteByte(MPL3115A2_ADDRESS, CTRL_REG1, tempSetting);

  tempSetting = IIC_ReadByte(MPL3115A2_ADDRESS, CTRL_REG1); //Read current settings to be safe
  tempSetting |= (1<<1); //Set OST bit
  IIC_WriteByte(MPL3115A2_ADDRESS, CTRL_REG1, tempSetting);
}
