// engler, cs140 put your gpio-int implementations in here.
#include "rpi.h"
#include "gpio-int.h"

// returns 1 if there is currently a GPIO_INT0 interrupt, 
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void) {
    int ans = (int)((GET32(IRQ_PENDING_2) & GPIO_INT0_IRQ_PIN) == GPIO_INT0_IRQ_PIN);
    return DEV_VAL32(ans);
}

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    if(pin >= 32)
        return;
    dev_barrier();

    //enabled gpio adresses before the interrupt controller - dev bar in b/w
    PUT32(GPREN0, GET32(GPREN0) | 1<<pin);
    dev_barrier();
    PUT32(ENABLE_IRQS_2, 1<<17); // 17 = 49-32
    dev_barrier();
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    if(pin >= 32)
        return;

    dev_barrier();
    PUT32(GPFEN0, GET32(GPFEN0) | 1<<pin);
    dev_barrier();
    PUT32(ENABLE_IRQS_2, 1 << 17); // 17 = 49-32
    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    if(pin >= 32)
        return -1;

    dev_barrier();
    int ans = (int)((GET32(GPEDS0) & 1<<pin) == 1<<pin);
    dev_barrier();
    
    return DEV_VAL32(ans);
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    if(pin >= 32)
        return;

    dev_barrier();
    PUT32(GPEDS0, 1<<pin);
    dev_barrier();
}
