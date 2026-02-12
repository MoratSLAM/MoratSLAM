#ifndef MPU6050_MORATO_H
#define MPU6050_MORATO_H

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

class MPU6050_Morato
{
    public:
        // Constructor
        MPU6050_Morato();
        
        // Public methods
        bool begin();
        bool update();
        float get_yaw_deg();
        void get_yaw_pitch_roll(float ypr_out[3]);
        Quaternion get_quaternion();
        VectorInt16 get_gyro();
        VectorInt16 get_accel();

    private:
        MPU6050 mpu;
        bool dmpReady;
        uint8_t devStatus;
        uint16_t packetSize;
        uint8_t fifoBuffer[64];
        Quaternion q;
        VectorFloat gravity;
        float ypr[3];
        VectorInt16 gyro;
        VectorInt16 accel;
};
#endif