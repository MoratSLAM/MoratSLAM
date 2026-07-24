#include <Arduino.h>
#include "ENCODER_Morato.h"

// Pin definitions for the encoder
#define ENCODER_A 32
#define ENCODER_B 33

// Constants for the encoder
const float wheel_perimeter_cm = 45.2389342f;
const float pulses_per_rev = 355.0f;

// Create an instance of the EncoderMorato class
EncoderMorato encoder(ENCODER_A, ENCODER_B, pulses_per_rev, wheel_perimeter_cm);

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("Inicializando Encoder...");
    encoder.begin();

    Serial.println("Encoder pronto!");
}

void loop()
{
    // Compute the velocity in cm/s
    float vel = encoder.compute_velocity();
    Serial.print("Vel [cm/s]: ");
    Serial.print(vel, 2);

    delay(100);
}