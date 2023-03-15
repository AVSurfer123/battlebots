// engler, cs140 put your gpio-int implementations in here.
#include "rpi.h"

enum {
    GPIO_BASE = 0x20200000,
    GPIO_EVENT_STATUS = (GPIO_BASE + 0x40),
    GPIO_RISING = (GPIO_BASE + 0x4C),
    GPIO_FALLING = (GPIO_BASE + 0x58),
    INTERRUPT_BASE = 0x2000B000,
    IRQ_PENDING2 = INTERRUPT_BASE + 0x208,
    ENABLE_IRQS_2 = INTERRUPT_BASE + 0x214,
    DISABLE_IRQS_2 = INTERRUPT_BASE + 0x220,
};

// returns 1 if there is currently a GPIO_INT0 interrupt, 
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void) {
    int val = (GET32(IRQ_PENDING2) >> 17) & 1;
    return DEV_VAL32(val);
}

static void gpio_enable_interrupt(unsigned pin) {
    dev_barrier();
    PUT32(ENABLE_IRQS_2, 1 << 17);
    dev_barrier();
}

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    if (pin >= 32) {
        return;
    }
    dev_barrier();
    PUT32(GPIO_RISING, GET32(GPIO_RISING) | 1 << pin);
    gpio_enable_interrupt(pin);
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    if (pin >= 32) {
        return;
    }
    dev_barrier();
    PUT32(GPIO_FALLING, GET32(GPIO_FALLING) | 1 << pin);
    gpio_enable_interrupt(pin);
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    if (pin >= 32) {
        return -1;
    }
    dev_barrier();
    int val = (GET32(GPIO_EVENT_STATUS) >> pin) & 1;
    dev_barrier();
    return DEV_VAL32(val);
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    if (pin >= 32) {
        return;
    }
    dev_barrier();
    PUT32(GPIO_EVENT_STATUS, 1 << pin);
    dev_barrier();
}
