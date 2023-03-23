#ifndef __ENCODERS_H__
#define __ENCODERS_H__

typedef struct
{
    int32_t position;
    int32_t velocity;
    int32_t counts;
    int32_t encState;
    int32_t lastTime;
} MotorData;

void get_encoder_velocity(MotorData* data, int A, int B);

#endif