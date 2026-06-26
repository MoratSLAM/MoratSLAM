#include <Arduino.h>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>

#include <nav_msgs/msg/odometry.h>
#include <sensor_msgs/msg/nav_sat_fix.h>
#include <rosidl_runtime_c/string_functions.h>

#include <OTA_ESP32.h>
#include <WiFi.h>

#include "sensors_manager.h"

// WiFi
/*
const char* WIFI_SSID     = "morato";
const char* WIFI_PASSWORD = "mor@to123";

// OTA
const char* DEVICE_HOSTNAME = "ESP32_SENSORS";
const char* OTA_PASSWORD    = "mor@to123";
*/



static double last_yaw = 0.0;
static int64_t last_time = 0;


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

// SENSORS OBJECT
SensorManager sensors;

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

  // Sensors initialization
  sensors.init(0, 0.00001, 50);

  // Configure micro-ROS transport over serial
  set_microros_serial_transports(Serial);

  // Initialize micro-ROS memory allocator
  allocator = rcl_get_default_allocator();

  // Initialize micro-ROS support structure
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Create the node
  RCCHECK(rclc_node_init_default(&node, "esp32_sensors_node", "", &support));

  
  // Sync time with ROS agent
  while (rmw_uros_sync_session(1000) != RMW_RET_OK)
  {
    delay(500);
  }

  // Initialize messages
  nav_msgs__msg__Odometry__init(&odom_msg);
  sensor_msgs__msg__NavSatFix__init(&gps_msg);

  // Set fixed frame names (done once)
  rosidl_runtime_c__String__assign(&odom_msg.header.frame_id, "odom");
  rosidl_runtime_c__String__assign(&odom_msg.child_frame_id, "base_link");

  // Create publishers
  RCCHECK(rclc_publisher_init_default(&odom_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry), "irat_red/odom"));

  RCCHECK(rclc_publisher_init_default(&gps_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, NavSatFix), "gps"));

  // Create timer callback (10 ms)
  RCCHECK(rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(200), timer_callback));

  // Configure executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  digitalWrite(2, LOW);
}

// LOOP
void loop()
{
  // OTA handler must be called continuously.
  // OtaManager::handle();

  // Sync time with ROS agent every 5 seconds
  static unsigned long last_sync = 0;
  if (millis() - last_sync > 5000)
  {
    rmw_uros_sync_session(100);
    last_sync = millis();
  }

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
  // Update sensor data
  sensors.update();

  //unsigned long start_micros = micros();  // Tempo inicial em microssegundos

  RCLC_UNUSED(last_call_time);
  if (timer == NULL) return;

  const SensorData& s = sensors.getData();

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

  int64_t gyro_dt = rmw_uros_epoch_nanos();

  double dt = (gyro_dt - last_time) / 1e9;

  double gyro_z = (s.yaw - last_yaw) / dt;

  last_yaw = s.yaw;
  last_time = gyro_dt;

  // Angular velocity
  odom_msg.twist.twist.angular.x = s.gyro_x;
  odom_msg.twist.twist.angular.y = s.gyro_y;
  odom_msg.twist.twist.angular.z = gyro_z * (-1.0);

/*
  unsigned long end_micros = micros();  // Tempo final em microssegundos

  double exec_time_ms = (end_micros - start_micros) / 1000.0;  // Tempo de execução em ms

  // Atualizar linear.y com o tempo de execução
  odom_msg.twist.twist.linear.y = exec_time_ms;

*/

  // Publish the odometry message
  RCSOFTCHECK(rcl_publish(&odom_pub, &odom_msg, NULL));

  // GPS
  gps_msg.header.stamp = odom_msg.header.stamp;
  gps_msg.latitude  = (float)s.latitude;
  gps_msg.longitude = (float)s.longitude;
  gps_msg.altitude  = s.satellites;

  // Publish GPS message
  RCSOFTCHECK(rcl_publish(&gps_pub, &gps_msg, NULL));
}