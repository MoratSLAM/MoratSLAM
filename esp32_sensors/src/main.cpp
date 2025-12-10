#include <Arduino.h>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <nav_msgs/msg/odometry.h>
#include <rosidl_runtime_c/string_functions.h>
#include <geometry_msgs/msg/point.h>
#include <sensor_msgs/msg/nav_sat_fix.h>

#include <MPU6050_Morato.h>

#include <TINY_GPS.h>

// GLOBALS
rcl_publisher_t odom_pub;
nav_msgs__msg__Odometry odom_msg;

rcl_publisher_t point_pub;
geometry_msgs__msg__Point point_msg;

rcl_publisher_t gps_pub;
sensor_msgs__msg__NavSatFix gps_msg;

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t odom_timer;

HardwareSerial GPS_Serial(2);
GPSLocalization gpsLoc(GPS_Serial, 115200, 16, 17);

// Macros de error-checking para micro-ROS
#define RCCHECK(fn)  { rcl_ret_t rc = fn; if(rc != RCL_RET_OK){ error_loop(); }}
#define RCSOFTCHECK(fn) { rcl_ret_t rc = fn; if(rc != RCL_RET_OK){}} // Isso aqui eu não sei se faz muito sentido pra gente

// Prototypes
void error_loop();
void timer_callback(rcl_timer_t * timer, int64_t last_call_time);


// SETUP
void setup()
{
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  Serial.begin(115200);
  delay(2000);

  // Configure micro-ROS transport over serial
  set_microros_serial_transports(Serial);

  // Initialize micro-ROS memory allocator
  allocator = rcl_get_default_allocator();

  // Initialize micro-ROS support structure
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Create the node
  RCCHECK(rclc_node_init_default(&node, "esp32_sensors_node","", &support));

  // Initialize messages
  nav_msgs__msg__Odometry__init(&odom_msg);
  sensor_msgs__msg__NavSatFix__init(&gps_msg);
  geometry_msgs__msg__Point__init(&point_msg);

  // Set fixed frame names (done once)
  rosidl_runtime_c__String__assign(&odom_msg.header.frame_id, "odom");
  rosidl_runtime_c__String__assign(&odom_msg.child_frame_id, "base_link");

  // Create odometry publisher
  RCCHECK(rclc_publisher_init_default(&odom_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry), "odom"));

  // Create gps publisher
  RCCHECK(rclc_publisher_init_default(&gps_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, NavSatFix), "gps"));

  // Create point publisher
  RCCHECK(rclc_publisher_init_default(&gps_pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Point), "point"));

  // Create timer callback (10 ms)
  RCCHECK(rclc_timer_init_default(&odom_timer, &support, RCL_MS_TO_NS(10), timer_callback));

  // Configure executor
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &odom_timer));

  // Initialize the MPU sensor
  MPU6050_Config();

  // Initialize GPS
  //gpsLoc.setReference(3, 0.00001, 50);

  digitalWrite(2, LOW);
}

// LOOP 
void loop()
{
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

// ODOMETRY CALLBACK 
void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{   
    RCLC_UNUSED(last_call_time);
    if (timer == NULL) return;

    /*----------------MPU PART----------------*/
    MPU6050_Read();

    // Set ROS2 timestamp
    odom_msg.header.stamp.sec = (int32_t)(millis() / 1000);
    odom_msg.header.stamp.nanosec = (millis() % 1000) * 1000000;

    // Position values
    odom_msg.pose.pose.position.x = 0.0;
    odom_msg.pose.pose.position.y = 0.0;
    odom_msg.pose.pose.position.z = 0.0;

    // Orientation as quaternion
    odom_msg.pose.pose.orientation.x = q.x;
    odom_msg.pose.pose.orientation.y = q.y;
    odom_msg.pose.pose.orientation.z = q.z;
    odom_msg.pose.pose.orientation.w = q.w;

    // Linear velocity
    odom_msg.twist.twist.linear.x = velX;
    odom_msg.twist.twist.linear.y = velY;
    odom_msg.twist.twist.linear.z = velZ;

    // Angular velocities
    odom_msg.twist.twist.angular.x = gyro.x;
    odom_msg.twist.twist.angular.y = gyro.y;
    odom_msg.twist.twist.angular.z = gyro.z;

    // Publish the odometry message
    RCSOFTCHECK(rcl_publish(&odom_pub, &odom_msg, NULL));

    /*----------------GPS PART----------------*/
    double lat, lon;
    double x, y;
    gpsLoc.update();

    lat = gpsLoc.getLatitude();
    lon = gpsLoc.getLongitude();

    gps_msg.latitude = (float)lat;
    gps_msg.longitude = (float)lon;

    // Set ROS2 timestamp
    gps_msg.header.stamp.sec = (int32_t)(millis() / 1000);
    gps_msg.header.stamp.nanosec = (millis() % 1000) * 1000000;

    // Publish the gps message
    RCSOFTCHECK(rcl_publish(&gps_pub, &gps_msg, NULL));

    gpsLoc.getXY(x, y);
    point_msg.x = (float)x;
    point_msg.y = (float)y;
    
    // Publish the point message
    RCSOFTCHECK(rcl_publish(&point_pub, &point_msg, NULL));
}
