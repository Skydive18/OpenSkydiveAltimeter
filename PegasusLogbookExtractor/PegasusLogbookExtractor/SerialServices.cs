using System;
using System.IO.Ports;
using System.Management;
using System.Threading;

namespace PegasusLogbookExtractor
{
    internal static class SerialServices
    {
        public static string GetSerialPort()
        {
            var rc = string.Empty;
            try
            {
                ManagementObjectSearcher searcher =
                    new ManagementObjectSearcher("root\\WMI",
                    "SELECT * FROM MSSerial_PortName");

                foreach (ManagementObject queryObj in searcher.Get())
                {
                    //If the serial port's instance name contains USB 
                    //it must be a USB to serial device
                    if (queryObj["InstanceName"].ToString().Contains("USB"))
                    {
                        if (string.IsNullOrEmpty(rc))
                            rc = (queryObj["PortName"]).ToString();
                        else
                            return string.Empty;
                    }
                }
            }
            catch (ManagementException e)
            {
                Console.WriteLine("An error occurred while querying for WMI data: " + e.Message);
                throw;
            }

            return rc;
        }

        public static void ToggleDTR(this SerialPort port)
        {
            if (port?.IsOpen == true)
            {
                port.RtsEnable = true;
                Thread.Sleep(50);
                port.DtrEnable = true; // 'DTR -
                Thread.Sleep(50);
                port.DtrEnable = false; // 'DTR +
                port.RtsEnable = false;
            }
        }
    }
}
