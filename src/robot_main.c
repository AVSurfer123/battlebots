#include "rpi.h"
#include "nrf.h"
#include "nrf-test.h"
#include "joy_def.h"
#include "timer-interrupt.h"
#include "test-interrupts.h"
#include "lock.h"
#include "encoders.h"
#include <limits.h>
#include "mbox.h"

#define LEFT_ENABLE 2
#define LEFT_A 4
#define LEFT_B 6

#define RIGHT_ENABLE 21
#define RIGHT_A 17
#define RIGHT_B 19

#define LEFT_ENC_A 3
#define LEFT_ENC_B 0

#define RIGHT_ENC_A 20
#define RIGHT_ENC_B 24

#define SPRAYER 12

nrf_t* client;

axis_state axes[6];

extern MotorData* leftEnc;
extern MotorData* rightEnc;
extern uint32_t last_time;
extern int left_count;
extern int right_count;
extern int spray_count;
extern int left_power;
extern int right_power;
static unsigned deadline_us = 100000000;

unsigned abs(int val) {
    if (val < 0) {
        return -val;
    }
    else {
        return val;
    }
}

#define AXIS_MIN 0
#define AXIS_MAX SHRT_MAX

// Maps val from [AXIS_MIN, AXIS_MAX] to [0, max]
int map(int val, int max) {
    if (val < AXIS_MIN) {
        val = AXIS_MIN;
    }
    if (val > AXIS_MAX) {
        val = AXIS_MAX;
    }
    return (val - AXIS_MIN) * max / (AXIS_MAX - AXIS_MIN);
}

#define TIMER_FREQ 1000000
#define PWM_FREQ 500
#define RESOLUTION 200
#define TIMER_NCYCLES TIMER_FREQ / PWM_FREQ / RESOLUTION

/**
 * Provide duty cycle for left and right wheels in range [-1, 1].
 */
void drive(int left, int right) {
    if (left < 0) {
        gpio_set_off(LEFT_A);
        gpio_set_on(LEFT_B);
    }
    else if (left > 0) {
        gpio_set_on(LEFT_A);
        gpio_set_off(LEFT_B);
    }
    else {
        gpio_set_on(LEFT_A);
        gpio_set_on(LEFT_B);
    }

    if (right < 0) {
        gpio_set_off(RIGHT_A);
        gpio_set_on(RIGHT_B);
    }
    else if (right > 0) {
        gpio_set_on(RIGHT_A);
        gpio_set_off(RIGHT_B);
    }
    else {
        gpio_set_on(RIGHT_A);
        gpio_set_on(RIGHT_B);
    }

    left_power = map(abs(left), RESOLUTION);
    right_power = map(abs(right), RESOLUTION);
    // printk("Motor power: %u %u\n", left_power, right_power);
}

static void noop(void *arg) {
  rpi_exit(0);
}

void notmain() {
    const unsigned oneMB = 1024 * 1024;
    kmalloc_init_set_start((void*)oneMB, oneMB);
    int_init();

    output("Serial number: %u\n", serialno_trunc());

    // ================ CODE EXAMPLE ============== //
    // for (int i = 0; i < 30; i++) {
    //   rpi_fork_rt(noop, NULL, deadline_us);
    // }
    // rpi_thread_start_preemptive(SCHEDULE_RTOS);
    // ================ CODE EXAMPLE ============== //

    client = client_mk_noack(client_addr, sizeof(js_event));

    gpio_set_output(LEFT_ENABLE);
    gpio_set_output(LEFT_A);
    gpio_set_output(LEFT_B);
    gpio_set_output(RIGHT_ENABLE);
    gpio_set_output(RIGHT_A);
    gpio_set_output(RIGHT_B);
    // gpio_set_output(SPRAYER);

    leftEnc = enc_init(3, 1);
    rightEnc = enc_init(24, 26);

    // From experimental testing, each cycle in this init function corresponds to 1 microsecond delay
    // If we want 500 Hz PWM with 100 resolution, naive implementation gives us 50,000 Hz clock which means 20 us delay between interrupts
    printk("Initializing timer to %u us delay\n", TIMER_NCYCLES);
    timer_interrupt_init(TIMER_NCYCLES);
    cpsr_int_enable();

    js_event event;
    size_t axis;
    int last_packet_time = timer_get_usec();

    while (1) {
        int ret = nrf_read_exact_noblk(client, &event, sizeof(js_event));
        if (ret != sizeof(js_event)) {
            // didn't get packet
        }
        else {
            last_packet_time = timer_get_usec();
            switch (event.type)
            {
                case JS_EVENT_BUTTON:
                    printk("Button %u %s\n", event.number, event.value ? "pressed" : "released");
                    break;
                case JS_EVENT_AXIS:
                    axis = get_axis_state(&event, axes);
                    printk("Axis %u at (%d, %d)\n", axis, axes[axis].x, axes[axis].y); 
                    break;
                default:
                    /* Ignore init events. */
                    // printk("Read type %d\n", event.type);
                    break;
            }
        }
        // Reset robot if connection dropped, no packet for 1 second
        if (timer_get_usec() - last_packet_time > 1000000) {
            drive(0, 0);
            clean_reboot();
        }

        // use 4 for dpad
        int forward = -axes[0].y;
        int turn = axes[0].x;
        drive(forward + turn, forward - turn);
        printk("Drive commands: forward %d turn %d\n", forward, turn);

        // drive(40000, 40000);
        // drive(total_wait * 5 + 5000, total_wait * 5 + 5000);
    }
    drive(0, 0);
}
