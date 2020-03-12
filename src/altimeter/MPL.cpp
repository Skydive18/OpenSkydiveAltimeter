/*
 MPL3115A2 Barometric Pressure Sensor Library
 */

#include <Wire.h>
#include "MPL.h"
#include "common.h"
#include "RTC.h"

extern Rtc rtc;

#ifdef TEST_JUMP_ENABLE
namespace {
    // Debug data
    const int data1[] PROGMEM = {
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
        571 ,  566 ,  560,   555,   550,   545,   540,   535,   530,   525,
        520,   515,   510,   505,   500,   495,   490,   485,   480,   475,
        470,   465,   460,   455,   450,   445,   440,   435,   430,   425,
        420,   415,   410,   405,   400,   395,   390,   385,   380,   375,
        370,   365,   360,   355,   350,   345,   340,   335,   330,   325,
        320,   315,   310,   305,   300,   295,   290,   285,   280,   275,
        270,   265,   260,   255,   250,   245,   240,   235,   230,   225,
        220,   215,   210,   205,   200,   195,   190,   185,   180,   175,
        170,   165,   160,   155,   150,   145,   140,   135,   130,   125,
        120,   115,   110,   105,   100,    95,    90,    85,    80,    75,
         70,    65,    60,    55,    50,    45,    40,    35,    30,    25,
         20,    15,    10,     5,     0
    };
}
#endif

MPL3115A2::MPL3115A2() {
    //Set initial values for private vars
#ifdef TEST_JUMP_ENABLE
    isDebug = false;
#endif
}

//Begin
/*******************************************************************************************/
//Start I2C communication
void MPL3115A2::begin(void) {
    IIC_WriteByte(MPL3115A2_ADDRESS, CTRL_REG1, (IIC_ReadByte(MPL3115A2_ADDRESS, CTRL_REG1) & B11000111) | B10101000);
    IIC_WriteByte(MPL3115A2_ADDRESS, PT_DATA_CFG, 0x07);
    ground_altitude = rtc.loadZeroAltitude();
}

void MPL3115A2::zero() {
    ground_altitude += readAltitude();
    rtc.saveZeroAltitude(ground_altitude);
}

#ifdef TEST_JUMP_ENABLE
void MPL3115A2::debugPrint() {
    debugAltitude = 0;
    isDebug = true;
    readPtr = -16;
}
#endif

//Returns the number of meters above sea level
//Returns -999 if no new data is available
int MPL3115A2::readAltitude() {
#ifdef TEST_JUMP_ENABLE
    if (isDebug) {
        if (readPtr < 0) {
            if (debugAltitude < (pgm_read_word(&data1[0]))) {
                debugAltitude += debugAltitude < 1400 ? 7 : 15;
            } else {
                readPtr++;
            }
        } else {
            if (!pgm_read_word(&data1[readPtr >> 1])) {
                isDebug = false;
            }
            else if (readPtr & 1) {
                debugAltitude = ((pgm_read_word(&data1[readPtr >> 1]) + pgm_read_word(&data1[(readPtr >> 1) + 1])) / 2);
            }
            else {
                debugAltitude = pgm_read_word(&data1[readPtr >> 1]);
            }
            readPtr++;
        }
        return debugAltitude;
    }
#endif
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

/*!
 * Calculates the pressure at sea level (in hPa) from the specified altitude
 * (in meters), and atmospheric pressure (in hPa).
 * @param  altitude      Altitude in meters
 * @param  atmospheric   Atmospheric pressure in hPa
 * @return The approximate pressure
 */
/*
float Adafruit_BMP280::seaLevelForAltitude(float altitude, float atmospheric) {
  // Equation taken from BMP180 datasheet (page 17):
  // http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

  // Note that using the equation from wikipedia can give bad results
  // at high altitude.  See this thread for more information:
  // http://forums.adafruit.com/viewtopic.php?f=22&t=58064
  return atmospheric / pow(1.0 - (altitude / 44330.0), 5.255);
}
*/
