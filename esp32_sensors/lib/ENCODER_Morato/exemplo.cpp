#include <Arduino.h>
#include "ENCODER_Morato.h"

#define ENCODER_A 32
#define ENCODER_B 33

const float wheel_perimeter_cm = 45.2389342f;
const float pulses_per_rev = 355.0f;

EncoderMorato encoder(ENCODER_A,
                      ENCODER_B,
                      pulses_per_rev,
                      wheel_perimeter_cm);

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
    // ===== LEITURA DO ENCODER =====
    float vel = encoder.compute_velocity();
    Serial.print("Velocidade [cm/s]: ");
    Serial.print(vel, 2);

    delay(100);
}