#include "MPU6050_Morato.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
MPU6050 mpu;

#define OUTPUT_READABLE_YAWPITCHROLL



// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3], yaw;           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector


// raw sensor measurements
int16_t ax, ay, az;
int16_t gx, gy, gz;

VectorInt16 aa;      // aceleração bruta
VectorInt16 aaReal;  // aceleração linear sem gravidade

float velX = 0, velY = 0, velZ = 0;



void MPU6050_Config() {

        // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
        Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
    #endif

    MPU6050_boot();
}

void MPU6050_boot(){
    // initialize device
    //Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();
    // verify connection
    //Serial.println(F("Testing device connections..."));
    //Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // load and configure the DMP
    devStatus = mpu.dmpInitialize();
    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(3.00000);
    mpu.setYGyroOffset(-60.00000);
    mpu.setZGyroOffset(12.00000);
    mpu.setZAccelOffset(896.00000); // 1688 factory default for my test chip
    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
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
    } 
}

/*void MPU6050_Read() {
     // if programming failed, don't try to do anything
    if (!dmpReady) return;
    // read a packet from FIFO
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) { // Get the Latest packet 


        #ifdef OUTPUT_READABLE_YAWPITCHROLL
            
            mpu.getAcceleration(&ax, &ay, &az);
            mpu.dmpGetLinearAccel(&ax, &ay, &az);
            mpu.getRotation(&gx, &gy, &gz);
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
            //Serial.print("yaw:");
            yaw = ypr[0] * 180/M_PI;
        #endif
    }
}
*/
void MPU6050_Read() {
    if (!dmpReady) return;

    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {

        #ifdef OUTPUT_READABLE_YAWPITCHROLL

            // Quaternion e gravidade
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
            yaw = ypr[0] * 180.0f / M_PI;

            // Leitura das acelerações brutas
            mpu.getAcceleration(&ax, &ay, &az);

            // Colocar aceleração bruta em aa
            aa.x = ax;
            aa.y = ay;
            aa.z = az;

            // Aceleração linear (remove gravidade usando DMP)
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);

            // Converte aceleração linear para m/s²
            float accX = (float)aaReal.x / 16384.0f * 9.80665f;
            float accY = (float)aaReal.y / 16384.0f * 9.80665f;
            float accZ = (float)aaReal.z / 16384.0f * 9.80665f;

            // Integração simples para velocidade
            static unsigned long lastTime = micros();
            unsigned long now = micros();
            float dt = (now - lastTime) / 1000000.0f;
            lastTime = now;

            velX += accX * dt;
            velY += accY * dt;
            velZ += accZ * dt;

            // OPTIONAL: leitura do giroscópio bruto (sem uso aqui)
            mpu.getRotation(&gx, &gy, &gz);

        #endif
    }
}
