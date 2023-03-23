#ifndef __ENCODERS_H__
#define __ENCODERS_H__

typedef struct
{
    int32_t position;
    int32_t velocity;
} MotorData;

MotorData get_encoder_velocity(int A, int B);

#endif