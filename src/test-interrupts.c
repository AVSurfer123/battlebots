// test code for checking the interrupts.
#include "test-interrupts.h"

volatile int n_interrupt;

static interrupt_fn_t interrupt_fn;

void interrupt_vector(unsigned pc) {
    dev_barrier();
    n_interrupt++;

    if(!interrupt_fn(pc))
        panic("should have no other interrupts?\n");

    dev_barrier();
}

#include "vector-base.h"

// initialize all the interrupt stuff.  client passes in the 
// gpio int routine <fn>
//
// make sure you understand how this works.
void test_startup(init_fn_t init_fn, interrupt_fn_t int_fn) {
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");
    output("\tImportant: must loop back (attach a jumper to) pins 20 & 21\n");

    // initialize.
    extern uint32_t _interrupt_table[];
    int_vec_init(_interrupt_table);

    gpio_set_output(out_pin);
    gpio_set_input(in_pin);

    init_fn();
    interrupt_fn = int_fn;

    // in case there was an event queued up.
    gpio_event_clear(in_pin);

    // start global interrupts.
    cpsr_int_enable();
}


/********************************************************************
 * falling edge.
 */

volatile int n_falling;

// check if there is an event, check if it was a falling edge.
int falling_handler(uint32_t pc) {
    // todo("implement this: return 0 if no rising int\n");
    if (gpio_event_detected(in_pin)) {
        if (gpio_read(in_pin) == 0) {
            gpio_event_clear(in_pin);
            n_falling++;
            return 1;
        }
    }
    return 0;
}

void falling_init(void) {
    gpio_write(out_pin, 1);
    gpio_int_falling_edge(in_pin);
}

/********************************************************************
 * rising edge.
 */

volatile int n_rising;

// check if there is an event, check if it was a rising edge.
int rising_handler(uint32_t pc) {
    // todo("implement this: return 0 if no rising int\n");
    if (gpio_event_detected(in_pin)) {
        if (gpio_read(in_pin) == 1) {
            gpio_event_clear(in_pin);
            n_rising++;
            return 1;
        }
    }
    return 0;
}

void rising_init(void) {
    gpio_write(out_pin, 0);
    gpio_int_rising_edge(in_pin);
}

/********************************************************************
 * rising edge.
 */

#include "timer-interrupt.h"

void timer_test_init(void) {
    // turn on timer interrupts.
    timer_interrupt_init(0x4);
}

int timer_test_handler(uint32_t pc) {
    dev_barrier();
    // todo("implement this by stealing pieces from 5-interrupts/0-timer-int");
    unsigned pending = GET32(IRQ_basic_pending);
    // printk("Pending: %d\n", pending);

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
     * Q: what happens, exactly, if we delete?
     */
    PUT32(arm_timer_IRQClear, 1);
    dev_barrier();
    return 1;
}
