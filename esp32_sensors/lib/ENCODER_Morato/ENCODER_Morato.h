#ifndef ENCODER_MORATO_H
#define ENCODER_MORATO_H

#include <Arduino.h>

class EncoderMorato
{
public:
    EncoderMorato(uint8_t pin_a, uint8_t pin_b, float pulses_per_rev, float wheel_perimeter_cm);

    void begin();

    // Call periodically (e.g. inside timer callback)
    float compute_velocity();

    // Get the distance traveled in centimeters
    float get_distance_cm();

    // Get the current encoder count
    int32_t get_count();

private:
    uint8_t pin_a;
    uint8_t pin_b;

    float pulses_per_rev;
    float wheel_perimeter_cm;

    volatile int32_t encoder_count;
    int32_t encoder_count_prev;

    unsigned long last_time_ms;

    static EncoderMorato* instance;
    static void IRAM_ATTR encoder_isr();
};

#endif
