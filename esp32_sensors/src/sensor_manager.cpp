#include "sensor_manager.h"

#include <MPU6050_Morato.h>
#include <TINY_GPS.h>
#include <ENCODER_Morato.h>

// ENCODER
#define ENCODER_A 32
#define ENCODER_B 33

static const float wheel_perimeter_cm = 45.2389342f;
static const float pulses_per_rev = 355.0f;

static EncoderMorato encoder(ENCODER_A, ENCODER_B, pulses_per_rev, wheel_perimeter_cm);

// GPS
static HardwareSerial GPS_Serial(2);
static GPS_localization gps(GPS_Serial, 115200, 16, 17);

// IMU
static MPU6050_Morato imu;

// DATA CACHE
static SensorData data;

void SensorManager::init()
{
    encoder.begin();
    imu.begin();
    gps.set_reference(10, 0.00001, 50);
}

void SensorManager::update()
{
    // Encoder
    data.velocity = encoder.compute_velocity();

    // IMU
    imu.update();

    Quaternion q = imu.get_quaternion();
    data.qx = q.x;
    data.qy = q.y;
    data.qz = q.z;
    data.qw = q.w;

    VectorInt16 gyro = imu.get_gyro();
    data.gyro_x = gyro.x;
    data.gyro_y = gyro.y;
    data.gyro_z = gyro.z;

    // GPS
    gps.update();

    data.latitude  = gps.get_latitude();
    data.longitude = gps.get_longitude();

    gps.get_xy(data.gps_x, data.gps_y);
    data.satellites = gps.get_satellites();
}

const SensorData& SensorManager::get_datas()
{
    return data;
}