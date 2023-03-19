#include "rpi.h"
#include "rpi-armtimer.h"
#include "rpi-interrupts.h"
#include "rpi-inline-asm.h"
#include "gpio-int.h"

#define PIN1 7
#define PIN2 13

uint32_t val1, val2;

void interrupt_vector(unsigned pc) {

    dev_barrier();

    if(gpio_event_detected(PIN1)){
        // gpio_read(PIN1);
        gpio_event_clear(PIN1);
        val1++;
    }
    if(gpio_event_detected(PIN2)){
        // gpio_read(PIN2);
        gpio_event_clear(PIN2);
        val2++;
    }

    // add specific code here

    dev_barrier();
    return;
}

void notmain() {
    printk("about to install handlers\n");
    int_init();

    printk("setting up gpio rising edge interrupt on pin %d\n", PIN1);
    gpio_set_input(PIN1);
    gpio_int_rising_edge(PIN1);
    // gpio_int_falling_edge(PIN1);
    gpio_event_clear(PIN1);

    printk("setting up gpio rising edge interrupt on pin %d\n", PIN2);
    gpio_set_input(PIN2);
    gpio_int_rising_edge(PIN2);
    // gpio_int_falling_edge(PIN2);
    gpio_event_clear(PIN2);

    system_enable_interrupts();

    uint32_t cpsr = cpsr_int_enable();

    val1 = 0;
    val2 = 0;
    int old_val = val1 + val2;

    while(1){
        if(old_val != val1 + val2){
            trace("there was an interrupt %d %d %d\n", old_val, val1, val2);
            old_val = val1 + val2;
        }
    }
}