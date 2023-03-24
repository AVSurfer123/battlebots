#pragma once

typedef struct
{
    uint8_t pinA;
    uint8_t pinB;
    int32_t position;
    int32_t velocity;
    int32_t counts;
    int32_t encState;
    int32_t lastTime;
} MotorData;

MotorData* enc_init(int pinA, int pinB);
void enc_callback(MotorData* data);
