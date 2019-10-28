/*
 MPL3115A2 Barometric Pressure Sensor Library
 */

#include <Wire.h>

#include "MPL.h"
#include "common.h"
#include "PCF8583.h"

/*
namespace {
    // Debug data
    int data1[] = {
        3078,  3192,  3041,  3124,  3114,  3067,  3031,  2988,  2939,  2899,
        2848,  2794,  2749,  2689,  2678,  2595,  2551,  2482,  2419,  2368,
        2307,  2247,  2203,  2140,  2112,  2036,  1979,  1920,  1865,  1806,
        1756,  1704,  1645,  1606,  1528,  1482,  1442,  1357,  1296,  1344,
        1165,  1071,  1032,  990 ,  979 ,  968 ,  959 ,  950 ,  942 ,  939,
        934 ,  926 ,  922 ,  920 ,  917 ,  913 ,  911 ,  908 ,  903 ,  899,
        896 ,  893 ,  889 ,  886 ,  881 ,  878 ,  875 ,  870 ,  867 ,  863,
        860 ,  857 ,  854 ,  849 ,  844 ,  839 ,  834 ,  829 ,  827 ,  822,
        817 ,  813 ,  810 ,  805 ,  801 ,  797 ,  793 ,  789 ,  785 ,  779,
        776 ,  771 ,  767 ,  764 ,  762 ,  758 ,  752 ,  748 ,  744 ,  741,
        735 ,  731 ,  727 ,  722 ,  718 ,  715 ,  713 ,  710 ,  703 ,  699,
        695 ,  693 ,  688 ,  684 ,  682 ,  678 ,  674 ,  669 ,  667 ,  660,
        658 ,  654 ,  649 ,  646 ,  641 ,  638 ,  634 ,  629 ,  625 ,  617,
        613 ,  609 ,  607 ,  603 ,  600 ,  595 ,  591 ,  587 ,  583 ,  578,
        571 ,  566, 0
    };
}
*/

MPL3115A2::MPL3115A2() {
    //Set initial values for private vars
//    isDebug = false;
}

//Begin
/*******************************************************************************************/
//Start I2C communication
void MPL3115A2::begin(void) {
    Wire.begin();
    IIC_WriteByte(MPL3115A2_ADDRESS, CTRL_REG1, (IIC_ReadByte(MPL3115A2_ADDRESS, CTRL_REG1) & B11000111) | B10101000);
    IIC_WriteByte(MPL3115A2_ADDRESS, PT_DATA_CFG, 0x07);
    ground_altitude = IIC_ReadInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE);
}

void MPL3115A2::zero() {
    ground_altitude = readAltitude();
    IIC_WriteInt(RTC_ADDRESS, ADDR_ZERO_ALTITUDE, ground_altitude);
}

/*
void MPL3115A2::debugPrint() {
    debugAltitude = groundAltitude = readAltitude();
    isDebug = true;
    readPtr = -16;
}
*/

//Returns the number of meters above sea level
//Returns -999 if no new data is available
int MPL3115A2::readAltitude() {
/*
    if (isDebug) {
        if (readPtr < 0) {
            if (debugAltitude < (data1[0] + groundAltitude)) {
                debugAltitude += 15;
            } else {
                readPtr++;
            }
        } else {
            if (!data1[readPtr >> 1]) {
                isDebug = false;
            }
            else if (readPtr & 1) {
                debugAltitude = ((data1[readPtr >> 1] + data1[(readPtr >> 1) + 1]) / 2) + groundAltitude;
            }
            else {
                debugAltitude = data1[readPtr >> 1] + groundAltitude;
            }
            readPtr++;
        }
        return debugAltitude;
    }
*/    
    toggleOneShot(); //Toggle the OST bit causing the sensor to immediately take another reading

    //Wait for PDR bit, indicates we have new pressure data
    uint16_t counter = 0;
    while( (IIC_ReadByte(MPL3115A2_ADDRESS, STATUS) & (1<<1)) == 0) {
        if(++counter > 600)
            return(-998); //Error out after max of 512ms for a read
        delay(1);
    }

    // Read pressure registers
    Wire.beginTransmission(MPL3115A2_ADDRESS);
    Wire.write(OUT_P_MSB);  // Address of data to get
    Wire.endTransmission(false); // Send data to I2C dev with option for a repeated start. THIS IS NECESSARY and not supported before Arduino V1.0.1!
    if (Wire.requestFrom(MPL3115A2_ADDRESS, 2) != 2) { // Request two bytes
        return -999;
    }

    byte msb, csb /*, lsb */;
    msb = Wire.read();
    csb = Wire.read();
    return ((msb << 8) | csb) - ground_altitude;
}

//Clears then sets the OST bit which causes the sensor to immediately take another reading
//Needed to sample faster than 1Hz
void MPL3115A2::toggleOneShot(void) {
    byte tempSetting = IIC_ReadByte(MPL3115A2_ADDRESS, CTRL_REG1); //Read current settings
    tempSetting &= ~(1<<1); //Clear OST bit
    IIC_WriteByte(MPL3115A2_ADDRESS, CTRL_REG1, tempSetting);

    tempSetting = IIC_ReadByte(MPL3115A2_ADDRESS, CTRL_REG1); //Read current settings to be safe
    tempSetting |= (1<<1); //Set OST bit
    IIC_WriteByte(MPL3115A2_ADDRESS, CTRL_REG1, tempSetting);
}
