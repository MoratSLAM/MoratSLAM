#pragma once

struct SensorData
{
    float velocity;

    float qx, qy, qz, qw;
    float gyro_x, gyro_y, gyro_z;

    double latitude;
    double longitude;

    double gps_x;
    double gps_y;
    int satellites;
};

namespace SensorManager
{
    void init();
    void update();
    const SensorData& get_datas();
}
