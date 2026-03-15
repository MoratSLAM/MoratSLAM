#include <Arduino.h>
#include <ESP32Servo.h>
#include <Bluepad32.h>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <geometry_msgs/msg/twist.h>
#include <std_msgs/msg/string.h>

// ================================
// SERVOS
// ================================

Servo SrvDir;
Servo EscMtr;
Servo SrvBrk;
Servo SrvGear;

const int DIR_LEFT = 41;
const int DIR_RIGHT = 141;
const int DIR_CENTER = 92;

const int ESC_MIN = 65;
const int ESC_STOP = 20;

const int BRK_ON = 41;
const int BRK_OFF = 141;

const int GEAR_FORWARD = 200;
const int GEAR_REVERSE = 60;


// ================================
// CONTROL MODE
// ================================

enum ControlMode
{
    MODE_JOYSTICK,
    MODE_AUTONOMOUS
};

ControlMode controlMode = MODE_JOYSTICK;


// ================================
// BLUEPAD
// ================================

GamepadPtr myGamepad;

bool lastOptions = false;
bool lastShare = false;


// ================================
// ROS VARIABLES
// ================================

rcl_node_t node;
rclc_support_t support;
rcl_allocator_t allocator;
rclc_executor_t executor;

rcl_subscription_t cmd_sub;

geometry_msgs__msg__Twist cmd_msg;

rcl_publisher_t debug_pub;
std_msgs__msg__String debug_msg;


// ================================
// ROS COMMAND STATE
// ================================

float ros_steer = 0.0;
float ros_throttle = 0.0;

unsigned long lastRosCmd = 0;

// Prototypes
void publishDebug(const char* text);
void error_loop();

// ================================
// ERROR MACROS
// ================================

#define RCCHECK(fn) { rcl_ret_t rc = fn; if(rc != RCL_RET_OK) error_loop(); }
#define RCSOFTCHECK(fn) { rcl_ret_t rc = fn; if(rc != RCL_RET_OK){} }


// ================================
// BLUEPAD CALLBACKS
// ================================

void onConnectedGamepad(GamepadPtr gp)
{
    myGamepad = gp;
}

void onDisconnectedGamepad(GamepadPtr gp)
{
    if (myGamepad == gp)
        myGamepad = nullptr;

    EscMtr.write(ESC_STOP);
}


// ================================
// ROS CALLBACK
// ================================

void cmd_callback(const void * msgin)
{
    const geometry_msgs__msg__Twist * msg =
        (const geometry_msgs__msg__Twist *)msgin;

    ros_steer = msg->angular.z;
    ros_throttle = msg->linear.x;

    lastRosCmd = millis();
}


// ================================
// DRIVE APPLY
// ================================

void applyDrive(float steer, float throttle)
{
    int dir = map(steer * 1000, -1000, 1000, DIR_LEFT, DIR_RIGHT);
    dir = constrain(dir, DIR_LEFT, DIR_RIGHT);

    SrvDir.write(dir);

    if (throttle > 0.1)
    {
        SrvBrk.write(BRK_OFF);
        EscMtr.write(ESC_MIN);
    }
    else
    {
        EscMtr.write(ESC_STOP);
        SrvBrk.write(BRK_ON);
    }
}


// ================================
// PROCESS GAMEPAD
// ================================

void processGamepad()
{
    if (!myGamepad || !myGamepad->isConnected())
        return;

    int lx = myGamepad->axisX();

    float steer = (float)lx / 511.0;

    bool accel = myGamepad->a();

    float throttle = accel ? 1.0 : 0.0;

    // DEBUG publish
    char debug_buffer[64];
    snprintf(debug_buffer, sizeof(debug_buffer),
             "JOY steer=%.2f throttle=%.2f",
             steer, throttle);

    publishDebug(debug_buffer);

    applyDrive(steer, throttle);

    // trocar modo

    bool options = myGamepad->miscButtons() & 0x02;
    bool share = myGamepad->miscButtons() & 0x01;

    if (options && !lastOptions)
    {
        controlMode = MODE_AUTONOMOUS;
    }

    if (share && !lastShare)
    {
        controlMode = MODE_JOYSTICK;
    }

    lastOptions = options;
    lastShare = share;
}


// ================================
// PROCESS ROS COMMANDS
// ================================

void processRosCommands()
{
    if (millis() - lastRosCmd > 500)
    {
        EscMtr.write(ESC_STOP);
        SrvBrk.write(BRK_ON);
        return;
    }

    applyDrive(ros_steer, ros_throttle);
}


// ================================
// DEBUG PUBLISH
// ================================

void publishDebug(const char* text)
{
    debug_msg.data.data = (char*)text;
    debug_msg.data.size = strlen(text);
    debug_msg.data.capacity = debug_msg.data.size + 1;

    RCSOFTCHECK(rcl_publish(&debug_pub, &debug_msg, NULL));
}


// ================================
// ERROR LOOP
// ================================

void error_loop()
{
    while (1)
    {
        delay(1000);
    }
}


// ================================
// SETUP
// ================================

void setup()
{
    Serial.begin(115200);

    SrvDir.attach(19);
    EscMtr.attach(16);
    SrvBrk.attach(17);
    SrvGear.attach(18);

    SrvDir.write(DIR_CENTER);
    SrvBrk.write(BRK_OFF);
    SrvGear.write(GEAR_FORWARD);

    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.enableVirtualDevice(false);


    // microROS

    set_microros_serial_transports(Serial);

    allocator = rcl_get_default_allocator();

    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    RCCHECK(rclc_node_init_default(&node, "esp32_drive_node", "", &support));


    RCCHECK(rclc_subscription_init_default(
        &cmd_sub,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "cmd_drive"));


    RCCHECK(rclc_publisher_init_default(
        &debug_pub,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
        "esp32_debug"));


    std_msgs__msg__String__init(&debug_msg);

    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));

    RCCHECK(rclc_executor_add_subscription(
        &executor,
        &cmd_sub,
        &cmd_msg,
        &cmd_callback,
        ON_NEW_DATA));
}


// ================================
// LOOP
// ================================

void loop()
{
    BP32.update();

    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));


    if (controlMode == MODE_JOYSTICK)
        processGamepad();

    if (controlMode == MODE_AUTONOMOUS)
        processRosCommands();
}