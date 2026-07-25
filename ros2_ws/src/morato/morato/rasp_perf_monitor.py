#!/usr/bin/env python3

"""
Raspberry Pi 4 Performance Monitor Node for ROS 2

This node collects and publishes critical hardware metrics to evaluate the 
embedded performance of the OpenRatslam2 algorithm during execution.

Metrics monitored:
1. Timestamp (s): ROS 2 clock time in seconds with sub-second precision.
2. General CPU Usage (%): Shows the total percentage of processing power being used.
3. RAM Usage (MB and %): Reveals the absolute and relative memory consumption.
4. CPU Temperature (°C): The most critical metric for the Raspberry Pi 4. The RPi 4 
   suffers from thermal throttling if it reaches 80 °C.
5. CPU Frequency (MHz): Serves to confirm if the board reduced its performance 
   (thermal throttling) due to heat during the test.
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64MultiArray  # Changed to Float64 to avoid timestamp precision loss
import psutil

class Pi4PerformanceMonitor(Node):
    def __init__(self):
        super().__init__('pi4_performance_monitor')
        
        # Publisher for system statistics using Float64MultiArray
        # Data layout index: 
        # [0] = Timestamp_sec
        # [1] = CPU_Usage_%
        # [2] = RAM_Usage_%
        # [3] = RAM_Usage_MB
        # [4] = CPU_Temp_C
        # [5] = CPU_Freq_MHz
        self.publisher_ = self.create_publisher(Float64MultiArray, '/pi4/system_stats', 10)
        
        # Timer to collect and publish data at 1 Hz (every 1.0 second)
        self.timer = self.create_timer(1.0, self.timer_callback)
        
        self.get_logger().info('Pi4 Performance Monitor Node has been started.')

    def timer_callback(self):
        msg = Float64MultiArray()
        
        # 0. Timestamp (ROS 2 Time in seconds as float)
        timestamp_sec = self.get_clock().now().nanoseconds / 1e9

        # 1. General CPU Usage (%)
        cpu_usage_pct = psutil.cpu_percent(interval=None)
        
        # 2. RAM Usage (MB and %)
        ram = psutil.virtual_memory()
        ram_usage_pct = ram.percent
        ram_usage_mb = ram.used / (1024.0 * 1024.0)
        
        # 3. CPU Temperature (°C)
        cpu_temp_c = 0.0
        try:
            with open("/sys/class/thermal/thermal_zone0/temp", "r") as temp_file:
                cpu_temp_c = float(temp_file.read()) / 1000.0
        except (FileNotFoundError, PermissionError):
            # Fallback for PC testing using psutil
            try:
                temps = psutil.sensors_temperatures()
                if temps:
                    for sensor_name, entries in temps.items():
                        if entries:
                            cpu_temp_c = float(entries[0].current)
                            break
            except Exception:
                cpu_temp_c = 0.0
            
        # 4. CPU Frequency (MHz)
        try:
            cpu_freq_mhz = psutil.cpu_freq().current
        except Exception:
            cpu_freq_mhz = 0.0

        # Assign collected data to the message array
        msg.data = [
            float(timestamp_sec),
            float(cpu_usage_pct), 
            float(ram_usage_pct), 
            float(ram_usage_mb), 
            float(cpu_temp_c), 
            float(cpu_freq_mhz)
        ]
        
        # Publish the message
        self.publisher_.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = Pi4PerformanceMonitor()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()

if __name__ == '__main__':
    main()