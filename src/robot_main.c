#include "rpi.h"
#include "nrf.h"
#include "nrf-test.h"
#include "joy_def.h"
#include "timer-interrupt.h"
#include "test-interrupts.h"
#include "encoders.h"
#include <limits.h>
#include "pwm.h"

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

MotorData* leftEnc;
MotorData* rightEnc;

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

int left_power = 0;
int right_power = 0;

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

uint32_t last_time = 0;
int left_count = 0;
int right_count = 0;
int spray_count = 0;

int timer_interrupt() {
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);

    // if this isn't true, could be a GPU interrupt (as discussed in Broadcom):
    // just return.  [confusing, since we didn't enable!]
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return 0;

    // Checkoff: add a check to make sure we have a timer interrupt
    // use p 113,114 of broadcom.

    /* 
     * Clear the ARM Timer interrupt - it's the only interrupt we have
     * enabled, so we don't have to work out which interrupt source
     * caused us to interrupt 
     *
     */
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();

    // uint32_t cur_time = timer_get_usec();
    // if (count % 10 == 0) {
    //     printk("Time diff: %u us\n", (cur_time - last_time));
    // }
    // last_time = cur_time;
    
    left_count += left_power;
    if (left_count >= RESOLUTION) {
        left_count -= RESOLUTION;
        gpio_set_on(LEFT_ENABLE);
    }
    else {
        gpio_set_off(LEFT_ENABLE);
    }

    right_count += right_power;
    if (right_count >= RESOLUTION) {
        right_count -= RESOLUTION;
        gpio_set_on(RIGHT_ENABLE);
    }
    else {
        gpio_set_off(RIGHT_ENABLE);
    }

    // spray_count += sprayer_angle;
    // if (right_count >= RESOLUTION) {
    //     right_count -= RESOLUTION;
    //     gpio_set_on(SPRAYER);
    // }
    // else {
    //     gpio_set_off(SPRAYER);
    // }

    dev_barrier();
    
    return 1;
}

void interrupt_vector(unsigned pc) {
    enc_callback(leftEnc);
    enc_callback(rightEnc);
    // printk("Left enc: %d, Right enc: %d\n", leftEnc->position, rightEnc->position);
    timer_interrupt();
}

void notmain() {
    int_init();

    client = client_mk_noack(client_addr, sizeof(js_event));

    gpio_set_output(LEFT_ENABLE);
    gpio_set_output(LEFT_A);
    gpio_set_output(LEFT_B);
    gpio_set_output(RIGHT_ENABLE);
    gpio_set_output(RIGHT_A);
    gpio_set_output(RIGHT_B);
    // gpio_set_output(SPRAYER);

    int pwm_val = 11;
    pwm_init(SPRAYER, pwm_val);

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
    int last_button = 0;

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
        
        if (event.type == JS_EVENT_BUTTON && event.number == 0) {
            if (event.value == 0 && last_button == 1) {
                pwm_val = 14 - pwm_val;
                pwm_set(SPRAYER, pwm_val);
            }            
            last_button = event.value;
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
