'''
To run this launch file without recording a rosbag:
    ros2 launch morato system_launch.py

To run this launch file AND record a rosbag:
    ros2 launch morato system_launch.py rec_bag:=true

To specify a different serial port and record:
    ros2 launch morato system_launch.py rec_bag:=true serial_port:=/dev/ttyUSB1
'''

import os
import re

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def get_bags_out_path():
    """
    Reads ~/.bashrc to find the ros2_ws path, scans the 'bags_out' 
    directory, and returns the next sequence path (morato_0, morato_1, morato_2...).
    """
    bashrc_path = os.path.expanduser('~/.bashrc')
    ws_path = None
    
    try:
        with open(bashrc_path, 'r') as f:
            lines = f.readlines()
            # Read from bottom to top to catch the last added setup.bash source
            for line in reversed(lines):
                match = re.search(r'source\s+(.*ros2_ws)/install/setup\.bash', line)
                if match:
                    ws_path = match.group(1)
                    break
    except Exception as e:
        print(f"Warning: Could not read bashrc file: {e}")
        
    if not ws_path:
        print("Warning: ros2_ws path not found in bashrc. Falling back to default path.")
        ws_path = os.path.expanduser('~/fallback_bags_out')  # Fallback path if not found in bashrc
        
    bags_dir = os.path.join(ws_path, 'bags_out')
    
    # Create the 'bags_out' directory if it does not exist
    os.makedirs(bags_dir, exist_ok=True)
    
    # Search for directories named strictly with 'morato_<NUMBER>' (e.g., morato_0, morato_1...)
    prefix = "morato_"
    existing_numbers = []
    
    for entry in os.listdir(bags_dir):
        full_path = os.path.join(bags_dir, entry)
        if os.path.isdir(full_path) and entry.startswith(prefix):
            suffix = entry[len(prefix):]
            if suffix.isdigit():
                existing_numbers.append(int(suffix))
            
    # Determine the next sequence index (starts at 0 if no matching directories exist)
    next_index = max(existing_numbers) + 1 if existing_numbers else 0
    bag_folder_name = f"{prefix}{next_index}"
    
    final_path = os.path.join(bags_dir, bag_folder_name)
    print(f"[LAUNCH] New bag will be saved to: {final_path}")
    return final_path


def launch_setup(context, *args, **kwargs):
    """
    Evaluates runtime configuration arguments and prepares the node execution list.
    """
    # Check if rec_bag argument was set to true/1/yes
    rec_bag_str = LaunchConfiguration('rec_bag').perform(context)
    should_record = rec_bag_str.lower() in ['true', '1', 'yes']

    # 1. Micro-ROS Agent Node
    micro_ros_node = Node(
        package='micro_ros_agent',
        executable='micro_ros_agent',
        name='micro_ros_agent',
        arguments=['serial', '--dev', LaunchConfiguration('serial_port')],
        output='screen',
        emulate_tty=True
    )

    # 2. Camera / Images Node
    get_images_node = Node(
        package='morato',
        executable='get_images',
        name='get_images',
        output='screen',
        emulate_tty=True
    )

    # 3. Raspberry Pi Hardware Performance Monitor Node
    perf_monitor_node = Node(
        package='morato',
        executable='rasp_perf_monitor',
        name='rasp_perf_monitor',
        output='screen',
        emulate_tty=True
    )

    # Nodes to launch by default
    launch_nodes = [
        micro_ros_node,
        get_images_node,
        perf_monitor_node
    ]

    # Conditionally add the Rosbag Recording Process
    if should_record:
        bag_path = get_bags_out_path()
        record_bag_process = ExecuteProcess(
            cmd=['ros2', 'bag', 'record', '-a', '-s', 'sqlite3', '-o', bag_path],
            output='screen',
            emulate_tty=True
        )
        launch_nodes.append(record_bag_process)
    else:
        print("[LAUNCH] Rosbag recording is DISABLED (rec_bag:=false).")

    return launch_nodes


def generate_launch_description():
    # Declare launch arguments
    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyUSB0',
        description='Serial port for the micro-ROS agent'
    )

    rec_bag_arg = DeclareLaunchArgument(
        'rec_bag',
        default_value='false',
        description='Set to true to record all ROS 2 topics into a rosbag'
    )

    return LaunchDescription([
        serial_port_arg,
        rec_bag_arg,
        OpaqueFunction(function=launch_setup)
    ])