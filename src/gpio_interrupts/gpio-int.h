#ifndef __GPIO_INT_H__
#define __GPIO_INT_H__

enum {
    GPIO_BASE = 0x20200000,
    GPEDS0 = (GPIO_BASE + 0x40),
    GPEDS1 = (GPIO_BASE + 0x44),
    GPREN0 = (GPIO_BASE + 0x4C),
    GPREN1 = (GPIO_BASE + 0x50),
    GPFEN0 = (GPIO_BASE + 0x58),
    GPFEN1 = (GPIO_BASE + 0x5C),
    IRQ_PENDING_2 = 0x2000B208,
    ENABLE_IRQS_2 = 0x2000B214,
};

#define GPIO_INT0_GPIO_PIN 0
#define GPIO_INT0_IRQ_PIN 0x2000


// returns 1 if there is currently a GPIO_INT0 interrupt, 
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void);

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin);

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin);

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin);

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin);

#endif