#include "ENCODER_Morato.h"

EncoderMorato* EncoderMorato::instance = nullptr;

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

void EncoderMorato::begin()
{
    pinMode(pin_a, INPUT_PULLUP);
    pinMode(pin_b, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(pin_a), EncoderMorato::encoder_isr, RISING);

    last_time_ms = millis();
}

void IRAM_ATTR EncoderMorato::encoder_isr()
{
    int b_state = digitalRead(instance->pin_b);

    if (b_state == HIGH)
        instance->encoder_count++;
    else
        instance->encoder_count--;
}

float EncoderMorato::compute_velocity()
{
    unsigned long now_ms = millis();
    float dt = (now_ms - last_time_ms) / 1000.0f;

    if (dt <= 0.0f)
        return 0.0f;

    int32_t delta = encoder_count - encoder_count_prev;
    encoder_count_prev = encoder_count;
    last_time_ms = now_ms;

    return ((delta / pulses_per_rev) * wheel_perimeter_cm) / dt;
}

int32_t EncoderMorato::get_count()
{
    return encoder_count;
}

float EncoderMorato::get_distance_cm()
{
    return (encoder_count / pulses_per_rev) * wheel_perimeter_cm;
}