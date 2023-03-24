#include "rpi.h"
#include "encoders.h"
#include "timer-interrupt.h"
#include "rpi-interrupts.h"

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

#define RESOLUTION 200

MotorData* leftEnc = NULL;
MotorData* rightEnc = NULL;
uint32_t last_time = 0;
int left_count = 0;
int right_count = 0;
int spray_count = 0;
int left_power = 0;
int right_power = 0;

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

void ashwin_interrupt(unsigned pc) {
    enc_callback(leftEnc);
    enc_callback(rightEnc);
    // printk("Left enc: %d, Right enc: %d\n", leftEnc->position, rightEnc->position);
    timer_interrupt();
}

