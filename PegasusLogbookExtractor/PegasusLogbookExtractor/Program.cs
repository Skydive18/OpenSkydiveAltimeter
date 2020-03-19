using System;
using System.Collections.Generic;
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
            SerialPort serialPort = new SerialPort();
            serialPort.PortName = "COM18";
            serialPort.Open();
            // Toggle DTR

            serialPort.RtsEnable = true;

            System.Threading.Thread.Sleep(1000);

            serialPort.DtrEnable = true; // 'DTR -
            System.Threading.Thread.Sleep(50);

            serialPort.DtrEnable = false; // 'DTR +
            serialPort.RtsEnable = false;

            StringBuilder sb = new StringBuilder();
            string data = string.Empty;

            for (int i = 0; i < 10; ++i)
            {
                serialPort.Write("?");
                Thread.Sleep(200);
            }

            do
            {
                data = serialPort.ReadLine();
                sb.Append(data + "\n");
            } while (data != "PEGASUS_END");
            string result = sb.ToString();

            var outputFile = File.CreateText("pegasus.raw");
            outputFile.WriteLine(result);
            outputFile.Close();
        }
    }
}
