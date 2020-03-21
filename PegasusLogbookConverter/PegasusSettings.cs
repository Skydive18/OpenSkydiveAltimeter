using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace PegasusLogbookConverter
{
    /*
        uint16_t batt_min_voltage; // Factory settings: min battery voltage, in items
        uint8_t batt_multiplier;
        uint8_t unused1;
        uint8_t unused2;
        uint8_t unused3;
        uint8_t contrast : 4;
        uint8_t jump_profile_number : 4;
        // Flags byte 1
        uint8_t display_rotation : 1;
        uint8_t zero_after_reset : 1;
        uint8_t auto_power_off : 2;
        uint8_t backlight : 2;
        uint8_t use_led_signals : 1;
        uint8_t use_audible_signals : 1;
        //
        int16_t target_altitude;
        //
        uint8_t volume:2;
        uint8_t sound_amplifier_power_on:1; // this value turns sound voltage gainer / amplifier ON
        uint8_t precision_in_freefall:2; // precision in freefall: 0=no rounding, 1=10m, 2=50m, 3=100m
        uint8_t display_flipped:1; // 0 here means display is soldered in flipped position
        uint8_t sound_amplifier_power_polarity:1; // write this to PB6 to turn sound amplifier power ON
        //
        uint8_t volumemap[4];
        uint16_t stored_jumps;
        uint16_t stored_snapshots;
    */
    public class PegasusSettings
    {
        private byte data1 = 0xff;
        private byte flags1 = 0xff;
        private byte flags2 = 0xff;

        public Int16 batt_min_voltage { get; set; } = -1;
        public byte batt_multiplier { get; set; } = 0xff;

        public byte contrast
        {
            get
            {
                return (byte)(data1 & 0xf);
            }
            set
            {
                data1 = (byte)((data1 & 0xf0) | (value & 0xf));
            }
        }

        public byte jump_profile_number
        {
            get
            {
                return (byte)((data1 >> 4) & 0xf);
            }
            set
            {
                data1 = (byte)((data1 & 0xf) | ((value & 0xf) << 4));
            }
        }

        public bool display_rotation
        {
            get
            {
                return (flags1 & 1) != 0;
            }
            set
            {
                unchecked
                {
                    flags1 &= (byte)(~1);
                    if (value)
                    {
                        flags1 |= 1;
                    }
                }
            }
        }

        public bool zero_after_reset
        {
            get
            {
                return (flags1 & (1 << 1)) != 0;
            }
            set
            {
                unchecked
                {
                    flags1 &= (byte)(~ (1 << 1));
                    if (value)
                    {
                        flags1 |= (1 << 1);
                    }
                }
            }
        }

        public bool use_led_signals
        {
            get
            {
                return (flags1 & (1 << 6)) != 0;
            }
            set
            {
                unchecked
                {
                    flags1 &= (byte)(~(1 << 6));
                    if (value)
                    {
                        flags1 |= (1 << 6);
                    }
                }
            }
        }

        public bool use_audible_signals
        {
            get
            {
                return (flags1 & (1 << 7)) != 0;
            }
            set
            {
                unchecked
                {
                    flags1 &= (byte)(~(1 << 7));
                    if (value)
                    {
                        flags1 |= (1 << 7);
                    }
                }
            }
        }
    }
}
