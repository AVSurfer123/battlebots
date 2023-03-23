#include "rpi.h"
#include "encoders.h"
#include "cycle-count.h"

int8_t enc_table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

void get_encoder_velocity(MotorData* data, int A, int B)
{
    int new_val = A << 1 | B;
    // shift new value to old
    data->encState = data->encState << 2;
    data->encState = data->encState | new_val;

    int oldTotalCount = data->position;
    data->position += enc_table[data->encState & 0b1111];
    int deltaCount = data->position - oldTotalCount;

    data->velocity = deltaCount / (timer_get_usec() - data->lastTime);
    data->lastTime = timer_get_usec();
}
