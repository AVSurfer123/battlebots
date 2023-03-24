#include "rpi.h"
#include "encoders.h"
#include "cycle-count.h"

int8_t enc_table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

void update_encoder_counts(MotorData* data, int A, int B)
{
    int new_val = A << 1 | B;
    // shift new value to old
    data->encState = data->encState << 2;
    data->encState = data->encState | new_val;

    // int oldTotalCount = data->position;
    data->position += enc_table[data->encState & 0b1111];

    // int deltaCount = data->position - oldTotalCount;
    // data->velocity = deltaCount / (timer_get_usec() - data->lastTime);
    // data->lastTime = timer_get_usec();
}

void enc_callback(MotorData* data) {
    int A = gpio_read(data->pinA);
    int B = gpio_read(data->pinB);
    if (gpio_event_detected(data->pinA)) {
        gpio_event_clear(data->pinA);
    }
    if (gpio_event_detected(data->pinB)) {
        gpio_event_clear(data->pinB);
    }
    update_encoder_counts(data, A, B);
}

MotorData* enc_init(int pinA, int pinB) {
    gpio_set_input(pinA);
    gpio_set_input(pinB);
    gpio_set_off(pinA);
    gpio_set_off(pinB);
    gpio_int_falling_edge(pinA);
    gpio_int_rising_edge(pinA);
    gpio_int_falling_edge(pinB);
    gpio_int_rising_edge(pinB);

    MotorData* data = kmalloc(sizeof(MotorData));
    if (data == NULL) {
        panic("enc_init: kmalloc failed to allocate\n");
    }
    data->pinA = pinA;
    data->pinB = pinB;
    return data;
}
