using System;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;

namespace PegasusLogbookExtractor
{
    class Program
    {
        static void Main(string[] args)
        {
            // Memory for data

            byte[] rom = new byte[1024];
            byte[] flash = new byte[65536];

            bool fromFile = false;
            string serialPortName = string.Empty;

            if ((args.GetLength(0) > 0 && args[0] == "file"))
            {
                Console.WriteLine($"Use pegasus.raw");
                fromFile = true;
            }
            else
            {
                serialPortName = SerialServices.GetSerialPort();
                if (string.IsNullOrEmpty(serialPortName) && args.GetLength(0) > 0)
                {
                    serialPortName = args[0].ToUpper();
                }
                if (!serialPortName.StartsWith("COM"))
                {
                    string codeBaseUri = System.Reflection.Assembly.GetExecutingAssembly().CodeBase;
                    string codeBase = new Uri(codeBaseUri).Segments.ToList().Last();// LocalPath;
                    Console.WriteLine($"Usage: {codeBase} COMn\nPress any key to exit.");
                    Console.ReadKey();
                    return;
                }
                Console.WriteLine($"Use port {serialPortName}.");
            }

            //return;

            StringBuilder sb = new StringBuilder();

            int flash_lines = -1;
            int line = -1;

            bool store_to_flash = false;
            bool store_to_rom = false;

            uint logbook_addr = 0;
            int logbook_count = 0;
            uint snapshot_addr = 0;
            int snapshot_length = 0;
            int snapshot_count = 0;

//            using (SerialPort serialPort = new SerialPort())
            {
                SerialPort serialPort = null;
                StreamReader inputFile = null;
                if (fromFile)
                {
                    inputFile = File.OpenText("pegasus.raw");
                }
                else
                {
                    serialPort = new SerialPort();
                    serialPort.PortName = serialPortName;
                    serialPort.Open();
                    Console.WriteLine("Resetting altimeter and initiate data transfer.");
                    serialPort.ToggleDTR();

                    for (int i = 0; i < 25; ++i)
                    {
                        serialPort.Write("?");
                        Thread.Sleep(200);
                        if (serialPort.BytesToRead > 0)
                            break;
                    }
                    if (serialPort.BytesToRead == 0)
                    {
                        Console.WriteLine("Cannot initiate data transfer.\nPress any key to exit.");
                        serialPort.ToggleDTR();
                        Console.ReadKey();
                        return;
                    }
                }


                Console.WriteLine("Start data transfer.");
                string data = string.Empty;
                do
                {
                    if (fromFile)
                        data = inputFile.ReadLine();
                    else
                        data = serialPort.ReadLine();
                    if (string.IsNullOrWhiteSpace(data))
                        continue;
                    if (!fromFile)
                        sb.Append(data + "\n");
                    var parts = data.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                    switch (parts[0])
                    {
                        case "PLATFORM":
                            {
                                Console.WriteLine($"Hardware platform {parts[1]}");
                                switch (parts[1][2])
                                {
                                    case '7': flash_lines = 64 * 64; break;
                                    case '6': flash_lines = 32 * 64; break;
                                    case '5': flash_lines = 16 * 64; break;
                                    case '4': flash_lines = 8 * 64; break;
                                    case '3': flash_lines = 4 * 64; break;
                                }
                            }
                            break;
                        case "FLASH_BEGIN":
                            line = 0;
                            Console.WriteLine("Reading FLASH");
                            store_to_flash = true;
                            break;
                        case "FLASH_END":
                            store_to_flash = false;
                            break;
                        case "ROM_BEGIN":
                            Console.WriteLine("Reading NVRAM...");
                            store_to_rom = true;
                            break;
                        case "ROM_END":
                            store_to_rom = false;
                            break;
                        case "VERSION":
                            Console.WriteLine($"Software version {parts[1]}");
                            break;
                        case "PEGASUS_BEGIN":
                        case "PEGASUS_END":
                        case "LANG":
                            break;
                        case "LOGBOOK":
                            {
                                var parts2 = data.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                                if (parts2.Length != 4)
                                    break;
                                logbook_addr = Convert.ToUInt32($"0x{parts2[2]}", 16);
                                logbook_count = Convert.ToInt32(parts2[3]);
                            }
                            break;
                        case "SNAPSHOT":
                            {
                                var parts2 = data.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                                if (parts2.Length != 5)
                                    break;
                                snapshot_addr = Convert.ToUInt32($"0x{parts2[2]}", 16);
                                snapshot_count = Convert.ToInt32(parts2[3]);
                                snapshot_length = Convert.ToInt32(parts2[4]);
                            }
                            break;
                        default:
                            if (store_to_rom)
                                StoreData(rom, data);
                            else if (store_to_flash)
                                StoreData(flash, data);
                            break;

                    }
                    if (line >= 0 && flash_lines > 0)
                    {
                        line++;
                        if ((line & 32) == 0)
                        {
                            Console.Write($"{((float)line / (float)flash_lines) * 100:F1}% completed.\r");
                        }
                    }
                } while (data != "PEGASUS_END");
                serialPort.ToggleDTR();
            }

            Console.WriteLine("Data transfer complete.");

            if (!fromFile) {
                string result = sb.ToString();
                Console.WriteLine("Saving raw...");
                File.WriteAllText("pegasus.raw", result);
                Console.WriteLine("Saved to pegasus.raw.");
            }

            // Simple converter
            var jump_counter = rom[0] + (256 * rom[1]);
            if (jump_counter > 0)
            {
                Console.WriteLine($"Found {jump_counter} jumps in logbook.");
                StringBuilder jumpsb = new StringBuilder();
                jumpsb.Append("datetime\texit_altitude\tdeploy_altitude\tcanopy_altitude\tmax_freefall_speed_ms\tfreefall_time\ttotal_jump_time\tprofile\n");
                for (int i = 0; i < jump_counter; i++)
                {
                    UInt32 data1 = LoadUInt32(flash, logbook_addr + (i * 12));
                    UInt32 data2 = LoadUInt32(flash, logbook_addr + (i * 12) + 4);
                    UInt32 data3 = LoadUInt32(flash, logbook_addr + (i * 12) + 8);

                    uint year = (data1 & 0xfff);
                    uint month = ((data1 >> 12) & 0xf);
                    uint day = ((data1 >> 16) & 0x1f);
                    uint hour = ((data1 >> 21) & 0x1f);
                    uint minute = ((data1 >> 26) & 0x3f);

                    DateTime jumpDate = new DateTime((int)year, (int)month, (int)day, (int)hour, (int)minute, 0);

                    uint exit_altitude = 2 * (data2 & 0xfff);
                    uint deploy_altitude = 2 * ((data2 >> 12) & 0xfff);
                    uint max_freefall_speed_ms = ((data2 >> 24) & 0xff);

                    uint canopy_altitude = deploy_altitude - (2 * (data3 & 0x3ff));
                    uint freefall_time = ((data3 >> 10) & 0x1ff) / 2;
                    uint total_jump_time = ((data3 >> 19) & 0x7ff) / 2;
                    uint profile = ((data3 >> 30) & 0x3);
                    jumpsb.Append($"{jumpDate.ToString("yyyy-MM-dd HH:mm")}\t{exit_altitude}\t{deploy_altitude}\t{canopy_altitude}\t{max_freefall_speed_ms}\t{freefall_time}\t{total_jump_time}\t{profile}\n");
                }
                File.WriteAllText("pegasus.logbook", jumpsb.ToString());
                Console.WriteLine("Logbook saved to pegasus.logbook");
            }

            Console.WriteLine("Press any key to exit.");
            Console.ReadKey();
        }

        static void StoreData(byte[] where, string what)
        {
            var parts = what.Split(new char[] { ':', ' ' }, StringSplitOptions.RemoveEmptyEntries).Select(x => $"0x{x}").ToArray();
            if (parts.Length != 17)
                return;
            var addr = Convert.ToUInt16(parts[0], 16);
            for (int i = 0; i < 16; i++)
                where[addr + i] = Convert.ToByte(parts[i+1], 16);
        }

        static UInt32 LoadUInt32(byte[] from, long addr)
        {
            UInt32 rc = from[addr+3];
            rc <<= 8;
            rc += from[addr + 2];
            rc <<= 8;
            rc += from[addr + 1];
            rc <<= 8;
            rc += from[addr];
            return rc;
        }
    }

}
