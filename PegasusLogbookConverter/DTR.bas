Private Sub Form1_FormClosing(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing
    SerialPort1.Close()
End Sub
Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
    SerialPort1.PortName = "COM5"
    SerialPort1.Open()
End Sub
Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
    SerialPort1.RtsEnable = True

    Debug.WriteLine("DTR +")
    System.Threading.Thread.Sleep(1000)

    SerialPort1.DtrEnable = True 'DTR -
    Debug.WriteLine("DTR -")
    System.Threading.Thread.Sleep(1000)

    SerialPort1.DtrEnable = False 'DTR +
    Debug.WriteLine("DTR +")
    System.Threading.Thread.Sleep(1000)

    SerialPort1.RtsEnable = False
End Sub


ManagementObjectSearcher searcher = new ManagementObjectSearcher(
    "root\\CIMV2",
    "SELECT * FROM Win32_PnPEntity WHERE ClassGuid=\"{4d36e978-e325-11ce-bfc1-08002be10318}\""
);
foreach (ManagementObject queryObj in searcher.Get())
{
    // do what you like with the Win32_PnpEntity
}

To get all devices listed use this query instead:
"SELECT * FROM Win32_PnPEntity WHERE Name LIKE '%(COM[0-9]%'"

==============

//Below is code pasted from WMICodeCreator
try
{
    ManagementObjectSearcher searcher =
        new ManagementObjectSearcher("root\\WMI",
        "SELECT * FROM MSSerial_PortName");

    foreach (ManagementObject queryObj in searcher.Get())
    {
        Console.WriteLine("-----------------------------------");
        Console.WriteLine("MSSerial_PortName instance");
        Console.WriteLine("-----------------------------------");
        Console.WriteLine("InstanceName: {0}", queryObj["InstanceName"]);

        Console.WriteLine("-----------------------------------");
        Console.WriteLine("MSSerial_PortName instance");
        Console.WriteLine("-----------------------------------");
        Console.WriteLine("PortName: {0}", queryObj["PortName"]);

        //If the serial port's instance name contains USB 
        //it must be a USB to serial device
        if (queryObj["InstanceName"].ToString().Contains("USB"))
        {
            Console.WriteLine(queryObj["PortName"] + " 
            is a USB to SERIAL adapter/converter");
        }
    }
}
catch (ManagementException e)
{
    MessageBox.Show("An error occurred while querying for WMI data: " + e.Message);
} 

using System;
using System.Management;

public class Sample 
{    
    public static void Main() 
    {

        // Get the WMI class
        ManagementClass osClass = 
            new ManagementClass("Win32_OperatingSystem");

        osClass.Options.UseAmendedQualifiers = true;

        // Get the Properties in the class
        PropertyDataCollection properties =
            osClass.Properties;

        // display the Property names
        Console.WriteLine("Property Name: ");
        foreach (PropertyData property in properties)
        {
            Console.WriteLine(
                "---------------------------------------");
            Console.WriteLine(property.Name);
            Console.WriteLine("Description: " +
                property.Qualifiers["Description"].Value);
            Console.WriteLine();

            Console.WriteLine("Type: ");               
            Console.WriteLine(property.Type);

            Console.WriteLine();

            Console.WriteLine("Qualifiers: ");
            foreach(QualifierData q in 
                property.Qualifiers)
            {
                Console.WriteLine(q.Name);
            }
            Console.WriteLine();

            foreach (ManagementObject c in osClass.GetInstances())
            {
                Console.WriteLine("Value: ");
                Console.WriteLine(
                    c.Properties[property.Name.ToString()].Value);
        
                Console.WriteLine();
            }
        }    
    }
}
