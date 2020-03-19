using System;
using System.Management;

namespace SerialPortEnumerator
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                ManagementObjectSearcher searcher =
                    new ManagementObjectSearcher("root\\WMI",
                    "SELECT * FROM MSSerial_PortName");

                Console.WriteLine("==========================================");
                foreach (ManagementObject queryObj in searcher.Get())
                {
                    Console.WriteLine("InstanceName: {0}", queryObj["InstanceName"]);
                    foreach (var xitem in queryObj.Properties)
                    {
                        if (! "InstanceName".Equals(xitem.Name, StringComparison.CurrentCultureIgnoreCase ))
                            Console.WriteLine($"{xitem.Name} = {xitem.Value}");
                    }

                    //If the serial port's instance name contains USB 
                    //it must be a USB to serial device
                    if (queryObj["InstanceName"].ToString().Contains("USB"))
                    {
                        Console.WriteLine(queryObj["PortName"] + " is a USB to SERIAL adapter / converter");
                    }
                    Console.WriteLine("==========================================");
                }
            }
            catch (ManagementException e)
            {
                Console.WriteLine("An error occurred while querying for WMI data: " + e.Message);
            }
        }
    }
}
