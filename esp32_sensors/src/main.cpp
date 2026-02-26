#include <Arduino.h>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>

#include <nav_msgs/msg/odometry.h>
#include <geometry_msgs/msg/point.h>
#include <sensor_msgs/msg/nav_sat_fix.h>
#include <rosidl_runtime_c/string_functions.h>

#include <OTA_ESP32.h>
#include <WiFi.h>

#include "sensor_manager.h"

// WiFi
/*
const char* WIFI_SSID     = "morato";
const char* WIFI_PASSWORD = "mor@to123";

// OTA
const char* DEVICE_HOSTNAME = "ESP32_SENSORS";
const char* OTA_PASSWORD    = "mor@to123";
*/

// ROS OBJECTS
rcl_node_t node;
rclc_support_t support;
rcl_allocator_t allocator;
rclc_executor_t executor;
rcl_timer_t timer;

rcl_publisher_t odom_pub;
nav_msgs__msg__Odometry odom_msg;

rcl_publisher_t gps_pub;
sensor_msgs__msg__NavSatFix gps_msg;

rcl_publisher_t point_pub;
geometry_msgs__msg__Point point_msg;

// MACROS
#define RCCHECK(fn)  { rcl_ret_t rc = fn; if(rc != RCL_RET_OK){ error_loop(); }}
#define RCSOFTCHECK(fn) { rcl_ret_t rc = fn; if(rc != RCL_RET_OK){}} 

// PROTOTYPES
void error_loop();
void timer_callback(rcl_timer_t * timer, int64_t last_call_time);

// SETUP
void setup()
{
  //set_microros_wifi_transports((char *)ssid, (char *)password, agent_ip, agent_port);
  // LED for status
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  // Serial for debugging
  Serial.begin(115200);
  delay(2000);

  // OTA
  //OtaManager::begin(WIFI_SSID, WIFI_PASSWORD, DEVICE_HOSTNAME, OTA_PASSWORD);

  // Sensors
  SensorManager::init();

  // micro-ROS transport
  set_microros_serial_transports(Serial);

  // Configure micro-ROS transport over serial
  set_microros_serial_transports(Serial);

  // Initialize micro-ROS memory allocator
  allocator = rcl_get_default_allocator();

  // Initialize micro-ROS support structure
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Sync time with ROS agent
  rmw_uros_sync_session(1000);

  // Create the node
  RCCHECK(rclc_node_init_default(&node, "esp32_sensors_node", "", &support));

  // Initialize messages
  nav_msgs__msg__Odometry__init(&odom_msg);
  sensor_msgs__msg__NavSatFix__init(&gps_msg);
  geometry_msgs__msg__Point__init(&point_msg);

  // Set fixed frame names (done once)
  rosidl_runtime_c__String__assign(&odom_msg.header.frame_id, "odom");
  rosidl_runtime_c__String__assign(&odom_msg.child_frame_id, "base_link");

  // Create publishers
  RCCHECK(rclc_publisher_init_default(&odom_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry), "irat_red/odom"));

  RCCHECK(rclc_publisher_init_default(&gps_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, NavSatFix), "gps"));

  RCCHECK(rclc_publisher_init_default(&point_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Point), "point"));

  // Create timer callback (10 ms)
  RCCHECK(rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(10), timer_callback));

  // Configure executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  digitalWrite(2, LOW);
}

// LOOP
void loop()
{
  // OTA handler must be called continuously.
  OtaManager::handle();

  // Update sensor data
  SensorManager::update();

  // Process micro-ROS executor (timers, subscriptions, etc.)
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10)));
}

// ERROR LOOP
void error_loop()
{
  while (1)
  {
    Serial.println("Micro-ROS error!");
    digitalWrite(2, HIGH);
    delay(1000);
    digitalWrite(2, LOW);
    delay(1000);
  }
}

// TIMER CALLBACK
void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
  RCLC_UNUSED(last_call_time);
  if (timer == NULL) return;

  const SensorData& s = SensorManager::get_datas();

  // Timestamp
  int64_t now = rmw_uros_epoch_nanos();
  odom_msg.header.stamp.sec = now / 1000000000ULL;
  odom_msg.header.stamp.nanosec = now % 1000000000ULL;

  // Position
  odom_msg.pose.pose.position.x = 0.0;
  odom_msg.pose.pose.position.y = 0.0;
  odom_msg.pose.pose.position.z = 0.0;

  // Orientation as quaternion
  odom_msg.pose.pose.orientation.x = s.qx;
  odom_msg.pose.pose.orientation.y = s.qy;
  odom_msg.pose.pose.orientation.z = s.qz;
  odom_msg.pose.pose.orientation.w = s.qw;

  // Linear velocity
  odom_msg.twist.twist.linear.x = s.velocity;
  odom_msg.twist.twist.linear.y = 0.0;
  odom_msg.twist.twist.linear.z = 0.0;

  // Angular velocity
  odom_msg.twist.twist.angular.x = s.gyro_x;
  odom_msg.twist.twist.angular.y = s.gyro_y;
  odom_msg.twist.twist.angular.z = s.gyro_z;

  // Publish the odometry message
  RCSOFTCHECK(rcl_publish(&odom_pub, &odom_msg, NULL));

  // GPS
  gps_msg.header.stamp = odom_msg.header.stamp;
  gps_msg.latitude  = (float)s.latitude;
  gps_msg.longitude = (float)s.longitude;
  gps_msg.altitude  = 0.0;

  // Publish GPS message
  RCSOFTCHECK(rcl_publish(&gps_pub, &gps_msg, NULL));

  // LOCAL XY
  point_msg.x = (float)s.gps_x;
  point_msg.y = (float)s.gps_y;
  point_msg.z = (float)s.satellites;

  // Publish local XY point
  RCSOFTCHECK(rcl_publish(&point_pub, &point_msg, NULL));
}