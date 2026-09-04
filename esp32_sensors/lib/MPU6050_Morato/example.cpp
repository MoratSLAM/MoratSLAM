#include <Arduino.h>
#include "MPU6050_Morato.h"

MPU6050_Morato imu;

void setup() 
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("Initializing IMU...");

    if (!imu.begin()) 
    {
        Serial.println("Error starting MPU6050");
        while (true);
    }

    Serial.println("MPU6050 ready!");
}

void loop() {

    if (imu.update()) 
    {
        // ===== Yaw / Pitch / Roll =====
        float ypr[3];
        imu.get_yaw_pitch_roll(ypr);

        float yaw_deg   = ypr[0] * 180.0f / M_PI;
        float pitch_deg = ypr[1] * 180.0f / M_PI;
        float roll_deg  = ypr[2] * 180.0f / M_PI;

        Serial.print("Yaw: ");   Serial.print(yaw_deg);
        Serial.print(" | Pitch: "); Serial.print(pitch_deg);
        Serial.print(" | Roll: ");  Serial.println(roll_deg);

        // ===== Direct yaw in radians =====
        float yaw = imu.get_yaw_rad();
        Serial.print("Yaw (rad): ");
        Serial.println(yaw);

        // ===== Quaternion =====
        Quaternion q = imu.get_quaternion();
        Serial.print("Quat: ");
        Serial.print(q.w); Serial.print(", ");
        Serial.print(q.x); Serial.print(", ");
        Serial.print(q.y); Serial.print(", ");
        Serial.println(q.z);

        // ===== Gyroscope =====
        VectorInt16 gyro = imu.get_gyro();
        Serial.print("Gyro [raw]: ");
        Serial.print(gyro.x); Serial.print(", ");
        Serial.print(gyro.y); Serial.print(", ");
        Serial.println(gyro.z);

        // ===== Accelerometer =====
        VectorInt16 accel = imu.get_accel();
        Serial.print("Accel [raw]: ");
        Serial.print(accel.x); Serial.print(", ");
        Serial.print(accel.y); Serial.print(", ");
        Serial.println(accel.z);

        Serial.println("----------------------------");
        delay(100);
    }
}
