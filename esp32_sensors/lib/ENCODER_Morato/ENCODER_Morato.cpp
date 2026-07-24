#include "ENCODER_Morato.h"

EncoderMorato* EncoderMorato::instance = nullptr;

// Constructor
EncoderMorato::EncoderMorato(uint8_t pin_a, uint8_t pin_b, float pulses_per_rev, float wheel_perimeter_cm)
{
    this->pin_a = pin_a;
    this->pin_b = pin_b;
    this->pulses_per_rev = pulses_per_rev;
    this->wheel_perimeter_cm = wheel_perimeter_cm;

    encoder_count = 0;
    encoder_count_prev = 0;
    last_time_ms = 0;

    instance = this;
}

// Initialize the encoder
void EncoderMorato::begin()
{
    pinMode(pin_a, INPUT_PULLUP);
    pinMode(pin_b, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(pin_a), EncoderMorato::encoder_isr, RISING);

    last_time_ms = millis();
}

// Interrupt Service Routine (ISR) for the encoder
void IRAM_ATTR EncoderMorato::encoder_isr()
{
    instance->encoder_count++;
}

// Compute the velocity in cm/s
float EncoderMorato::compute_velocity()
{
    unsigned long now_ms = millis();
    float dt = (now_ms - last_time_ms) / 1000.0f;

    if (dt <= 0.0f)
        return 0.0f;

    noInterrupts();
    int32_t count = encoder_count;
    interrupts();

    int32_t delta = count - encoder_count_prev;
    
    encoder_count_prev = count;
    last_time_ms = now_ms;

    float wheel_perimeter_m = wheel_perimeter_cm / 100.0f;

    return (((float)delta / pulses_per_rev) * wheel_perimeter_m) / dt;
}

// Get the current encoder count
int32_t EncoderMorato::get_count()
{
    noInterrupts();
    int32_t count = encoder_count;
    interrupts();
    return count;
}

// Get the distance traveled in centimeters
float EncoderMorato::get_distance_cm()
{
    return (encoder_count / pulses_per_rev) * wheel_perimeter_cm;
}