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
            string serialPortName = SerialServices.GetSerialPort();
            if (string.IsNullOrEmpty(serialPortName) && args.GetLength(0) > 0)
            {
                serialPortName = args[0].ToUpper();
            }
            if (!serialPortName.StartsWith("COM"))
            {
                string codeBaseUri = System.Reflection.Assembly.GetExecutingAssembly().CodeBase;
                string codeBase = new Uri(codeBaseUri).Segments.ToList().Last() ;// LocalPath;
                Console.WriteLine($"Usage: {codeBase} COMn\nPress any key to exit.");
                Console.ReadKey();
                return;
            }
            Console.WriteLine($"Use port {serialPortName}.");

            //return;

            StringBuilder sb = new StringBuilder();

            int flash_lines = -1;
            int line = -1;

            using (SerialPort serialPort = new SerialPort()) {
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

                Console.WriteLine("Start data transfer.");
                string data = string.Empty;
                do
                {
                    data = serialPort.ReadLine();
                    if (string.IsNullOrWhiteSpace(data))
                        continue;
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
                            break;
                        case "ROM_BEGIN":
                            Console.WriteLine("Reading NVRAM...");
                            break;
                        case "VERSION":
                            Console.WriteLine($"Software version {parts[1]}");
                            break;
                    }
                    if (line >= 0 && flash_lines > 0)
                    {
                        line++;
                        if ((line & 32) == 0)
                        {
                            Console.Write($"{((float)line / (float)flash_lines)*100:F1}% completed.\r");
                        }
                    }
                } while (data != "PEGASUS_END");
                serialPort.ToggleDTR();
            }
            Console.WriteLine("Data transfer complete.");
            string result = sb.ToString();

            Console.WriteLine("Saving...");
            File.WriteAllText("pegasus.raw", result);

            Console.WriteLine("Saved to pegasus.raw\nPress any key to exit.");
            Console.ReadKey();
        }

    }

}
