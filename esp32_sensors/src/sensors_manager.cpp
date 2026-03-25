#include "sensors_manager.h"

// ENCODER
#define ENCODER_A 32
#define ENCODER_B 33

static const float wheel_perimeter_cm = 45.2389342f;
static const float pulses_per_rev = 355.0f;

// GPS
static HardwareSerial GPS_Serial(2);

SensorManager::SensorManager() :
    encoder(ENCODER_A, ENCODER_B, pulses_per_rev, wheel_perimeter_cm),
    gps(GPS_Serial, 115200, 16, 17),
    imu()
{
}

void SensorManager::init(int sat_threshold, double max_variation, int n_init_samples)
{
    encoder.begin();
    imu.begin();
    gps.set_reference(sat_threshold, max_variation, n_init_samples);
}

void SensorManager::update()
{
    data.velocity = encoder.compute_velocity();

    imu.update();

    data.yaw = imu.get_yaw_rad();

    Quaternion q = imu.get_quaternion();
    data.qx = q.x;
    data.qy = q.y;
    data.qz = q.z;
    data.qw = q.w;

    VectorInt16 gyro = imu.get_gyro();
    data.gyro_x = gyro.x;
    data.gyro_y = gyro.y;
    data.gyro_z = gyro.z;

    gps.update();

    data.latitude  = gps.get_latitude();
    data.longitude = gps.get_longitude();

    gps.get_xy(data.gps_x, data.gps_y);
    data.satellites = gps.get_satellites();
}

const SensorData& SensorManager::getData() const
{
    return data;
}