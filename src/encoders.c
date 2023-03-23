#include "rpi.h"
#include "encoders.h"

int8_t enc_table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

void encoder_interrupt()
{

    int new_val = A << 1 | B;

    // shift new to old
    enc_val = enc_val << 2;
    enc_val = enc_val | new_val;

    enc_count = enc_count + enc_table[enc_val & 0b1111];
}

int oldA, oldB;
int32_t totalCount;
int32_t velocity;

MotorData get_encoder_velocity(int A, int B)
{

    // oldA, oldB, A, B
    oldA = A;
    oldB = B;

    MotorData data;
    data.position = totalCount;
    data.velocity = velocity;
    return data;
}