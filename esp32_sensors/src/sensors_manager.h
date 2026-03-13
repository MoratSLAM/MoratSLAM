#pragma once

#include <MPU6050_Morato.h>
#include <TINY_GPS.h>
#include <ENCODER_Morato.h>

struct SensorData
{
    float velocity;

    float qx, qy, qz, qw;
    float gyro_x, gyro_y, gyro_z;
    float yaw;

    double latitude;
    double longitude;

    double gps_x;
    double gps_y;
    int satellites;
};

class SensorManager
{
    public:
        SensorManager();

        void init(int sat_threshold, double max_variation, int n_init_samples);
        void update();

        const SensorData& getData() const;

    private:
        EncoderMorato encoder;
        GPS_localization gps;
        MPU6050_Morato imu;

        SensorData data;
};