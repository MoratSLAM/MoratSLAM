#ifndef MPU6050_MORATO_H
#define MPU6050_MORATO_H

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

// Funções públicas
void MPU6050_Config();
void MPU6050_Read();
void MPU6050_boot();

// Variáveis úteis
extern float yaw;
extern VectorInt16 gyro;
extern Quaternion q;
extern float velX;
extern float velY;
extern float velZ;

#endif
