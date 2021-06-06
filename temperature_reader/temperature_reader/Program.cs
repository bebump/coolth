using System;
using System.Threading;
using OpenHardwareMonitor.Hardware;

namespace temperature_reader
{
    class Program
    {
        static void updateHardware(IHardware hw)
        {
            hw.Update();
            foreach (var subHw in hw.SubHardware)
            {
                subHw.Update();
            }
        }

        static void Main(string[] args)
        {
            float? cpuTemp = null;
            float? gpuTemp = null;

            var computer = new Computer() { CPUEnabled = true, GPUEnabled = true };
            computer.Open();

            while (true)
            {
                foreach (var hardwareItem in computer.Hardware)
                {
                    if (hardwareItem.HardwareType == HardwareType.CPU)
                    {
                        updateHardware(hardwareItem);

                        foreach (var sensor in hardwareItem.Sensors)
                        {
                            if (sensor.SensorType == SensorType.Temperature && sensor.Name == "CPU Package")
                            {
                                cpuTemp = sensor.Value;
                            }
                        }

                        if (cpuTemp == null)
                        {
                            foreach (var sensor in hardwareItem.Sensors)
                            {
                                if (sensor.SensorType == SensorType.Temperature && sensor.Name == "CPU Core #1")
                                {
                                    cpuTemp = sensor.Value;
                                }
                            }
                        }
                    }

                    if (hardwareItem.HardwareType == HardwareType.GpuAti || hardwareItem.HardwareType == HardwareType.GpuNvidia)
                    {
                        updateHardware(hardwareItem);
                        foreach (var sensor in hardwareItem.Sensors)
                        {
                            if (sensor.SensorType == SensorType.Temperature && sensor.Name == "GPU Core")
                            {
                                gpuTemp = sensor.Value;
                            }
                        }
                    }
                }

                var cpuTempString = cpuTemp.HasValue ? cpuTemp.Value.ToString() : "null";
                var gpuTempString = gpuTemp.HasValue ? gpuTemp.Value.ToString() : "null";
                Console.WriteLine($"{cpuTempString} {gpuTempString}");
                Thread.Sleep(1000);
            }
        }
    }
}
