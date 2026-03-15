cmake_minimum_required(VERSION 3.5)
project(topological_msgs)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
# find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

rosidl_generate_interfaces(topological_msgs
  "msg/TopologicalAction.msg"
  "msg/TopologicalEdge.msg"
  "msg/TopologicalMap.msg"
  "msg/TopologicalNode.msg"
  "msg/ViewTemplate.msg"
  "msg/InfoExp.msg"
  DEPENDENCIES geometry_msgs std_msgs builtin_interfaces
 )

ament_export_dependencies(rosidl_default_runtime)
ament_package()
