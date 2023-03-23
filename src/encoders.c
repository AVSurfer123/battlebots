#include "rpi.h"
#include "encoders.h"
#include "cycle-count.h"

int8_t enc_table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

int32_t totalCount;
int enc_val = 0;
unsigned lastCycleCount = 0;
MotorData get_encoder_velocity(int A, int B)
{
    int new_val = A << 1 | B;
    // shift new value to old
    enc_val = enc_val << 2;
    enc_val = enc_val | new_val;

    int oldTotalCount = totalCount;
    totalCount = totalCount + enc_table[enc_val & 0b1111];
    int deltaCount = totalCount - oldTotalCount;

    MotorData data;
    data.position = totalCount;
    data.velocity = deltaCount / (cycle_cnt_read() - lastCycleCount);
    lastCycleCount = cycle_cnt_read();
    return data;
}