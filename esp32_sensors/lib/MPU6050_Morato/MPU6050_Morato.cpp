#include "MPU6050_Morato.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

MPU6050_Morato::MPU6050_Morato()
    : dmpReady(false),
      devStatus(0),
      packetSize(0) {
}

bool MPU6050_Morato::begin()
{
    // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
        Wire.setClock(400000);
    #endif

    // initialize device
    mpu.initialize();

    // load and configure the DMP
    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(3.00000);
    mpu.setYGyroOffset(-60.00000);
    mpu.setZGyroOffset(12.00000);
    mpu.setZAccelOffset(896.00000);

    // make sure it worked (returns 0 if so)
    if (devStatus != 0) 
    {
        return false;
    }

    // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);

    //mpu.PrintActiveOffsets();
    // turn on the DMP, now that it's ready
    //Serial.println(F("Enabling DMP..."));

    mpu.setDMPEnabled(true);
    //mpuIntStatus = mpu.getIntStatus();

    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
    
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);

    return true;
}

bool MPU6050_Morato::update()
{
    // if programming failed, don't try to do anything
    if (!dmpReady) return false;

    // read a packet from FIFO
    if (!mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) return false;

    // display quaternion values in easy matrix form: w x y z
    mpu.dmpGetQuaternion(&q, fifoBuffer);

    // display gravity vector
    mpu.dmpGetGravity(&gravity, &q);

    // display Yaw/Pitch/Roll angles in radians
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    // display gyro and accel values
    mpu.dmpGetGyro(&gyro, fifoBuffer);

    // display accel values
    mpu.dmpGetAccel(&accel, fifoBuffer);

    return true;
}

float MPU6050_Morato::get_yaw_deg()
{
    return ypr[0]; //* 180.0f / M_PI;
}

void MPU6050_Morato::get_yaw_pitch_roll(float ypr_out[3])
{
    ypr_out[0] = ypr[0];
    ypr_out[1] = ypr[1];
    ypr_out[2] = ypr[2];
}

Quaternion MPU6050_Morato::get_quaternion()
{
    return q;
}

VectorInt16 MPU6050_Morato::get_gyro()
{
    return gyro;
}

VectorInt16 MPU6050_Morato::get_accel()
{
    return accel;
}