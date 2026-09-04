# MoratSLAM - RatSLAM on the Morato Robot

This repository contains the software, firmware, and hardware design files developed for the undergraduate thesis on applying visual RatSLAM-based navigation to the Morato autonomous mobile robot.

The project investigates how a neuroscience-inspired cognitive mapping system can be used for visual navigation, continuous localization, and topological mapping in real-world environments. The robot combines a Raspberry Pi, an ESP32-based sensor and motor system, a camera, wheel encoders, an IMU, and the ROS 2 implementation of OpenRatSLAM2.

The `rosbag2` recording used in the project is available in the [project's Google Drive folder](https://drive.google.com/drive/folders/1168A3YzXBIVLh-FVAhgKLTRrBHHMAMwr?usp=sharing). It can be used to reproduce and analyze the experiments described in the thesis.

## Repository Structure

```text
.
├── electronics/                 PCB design files and documentation
├── mechanical/                  Mechanical part files and documentation
├── esp32_motors/                ESP32 motor firmware
├── esp32_sensors/               ESP32 sensor firmware (PlatformIO)
├── microros_ws/                 micro-ROS setup submodule
├── plot_tools/                  Dataset and experiment plotting tools
└── ros2_ws/
	└── src/
		├── morato/               Robot drivers, monitoring, and visualization
		├── ratslam/               OpenRatSLAM2 ROS 2 package
		└── topological_msgs/      Custom ROS 2 messages
```

The `mechanical/` directory contains the files for the parts used in the project. The `electronics/` directory contains the PCB files. Both directories also contain a `docs/` directory intended for views, schematics, and related documentation.

The `esp32_sensors/` and `esp32_motors/` projects were developed in VS Code using the PlatformIO extension. Their project configuration files are already included, allowing PlatformIO to download the configured dependencies automatically. Any other custom or third-party libraries that are not listed in those configurations are included directly in the respective projects.

Most of the repository is essential to operating the robot. The only optional ROS 2 component is the `ratslam_visualizer` node in the `morato` package. It is not required on the robot: it can be run on a PC on the same network and with the same ROS Domain ID to visualize selected OpenRatSLAM2 information in RViz.

## License and Copyright

The original work in this repository is distributed under the GNU General Public License v3.0. The complete license text is available in [LICENSE](LICENSE).

The OpenRatSLAM2 package included at [`ros2_ws/src/ratslam`](ros2_ws/src/ratslam) also includes its own copy of the GNU General Public License v3.0 in [`ros2_ws/src/ratslam/LICENSE`](ros2_ws/src/ratslam/LICENSE). The license and copyright notices of that package and of the RatSLAM algorithm must be preserved when the package is used or modified.

All rights to the RatSLAM algorithm and its implementations are subject to the following notices and licenses:

```text
openRatSLAM Copyright (C) 2012 David Ball and Scott Heath
RatSLAM algorithm by Michael Milford and Gordon Wyeth
Distributed under the GNU GPL v3.
```

## Initialization

Clone the repository and initialize its Git submodule. The repository currently has one submodule: `microros_ws/src/micro_ros_setup`.

```bash
git clone https://github.com/MoratSLAM/MoratSLAM.git
cd MoratSLAM
git submodule update --init --recursive
```

Install the ROS 2 and system dependencies required by the packages. For the OpenRatSLAM2 dependencies, see [`ros2_ws/src/ratslam/README.md`](ros2_ws/src/ratslam/README.md). Then build and source the ROS 2 workspace:

```bash
cd ros2_ws
source /opt/ros/<ros-distro>/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

## Building micro-ROS

The micro-ROS tools and agent are built from the `microros_ws` workspace. From the repository root, run:

```bash
# Source the ROS 2 installation
source /opt/ros/$ROS_DISTRO/setup.bash

# Enter the micro-ROS workspace
cd microros_ws

# Update dependencies using rosdep
sudo apt update && rosdep update
rosdep install --from-paths src --ignore-src -y

# Build the micro-ROS tools and source them
colcon build
source install/local_setup.bash

# Download micro-ROS-Agent packages
ros2 run micro_ros_setup create_agent_ws.sh

# Build the micro-ROS agent
ros2 run micro_ros_setup build_agent.sh
source install/local_setup.bash
```

More information about `micro_ros_setup` can be found in its [official repository](https://github.com/micro-ROS/micro_ros_setup.git).

## Running the micro-ROS Agent

After building the agent and connecting the microcontroller, run:

```bash
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0
```

## Running the ROS 2 Nodes

Run the following commands from a sourced ROS 2 workspace. On the robot, use the ROS Domain ID configured for the robot. A visualization PC must use the same ROS Domain ID and be connected to the same network.

### OpenRatSLAM2

The `ratslam` package is normally started through one of its launchers. For the Morato robot, use:

```bash
ros2 launch ratslam morato.launch.py
```

### Morato

The complete robot-side Morato system can be started with:

```bash
ros2 launch morato system_launch.py
```

This starts the micro-ROS agent, the camera/image node, and the Raspberry Pi performance monitor. To record all ROS 2 topics to a rosbag at the same time:

```bash
ros2 launch morato system_launch.py rec_bag:=true
```

To select a different micro-ROS serial port:

```bash
ros2 launch morato system_launch.py serial_port:=/dev/ttyACM0
```

The individual Morato nodes can also be run directly when only one function is needed:

```bash
ros2 run morato get_images
ros2 run morato rasp_perf_monitor
ros2 run morato ratslam_visualizer
```

`get_images` and `rasp_perf_monitor` are robot-side nodes. `ratslam_visualizer` is optional and is intended to run on a networked PC with RViz; it is not required for the robot to operate.

## Related Documentation

- [OpenRatSLAM2 package README](ros2_ws/src/ratslam/README.md)
- [Repository license](LICENSE)
- [OpenRatSLAM2 package license](ros2_ws/src/ratslam/LICENSE)
