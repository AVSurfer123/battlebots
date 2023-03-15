/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),
    gpio_pud = (GPIO_BASE + 0x94),
    gpio_pudclk = (GPIO_BASE + 0x98),
};


// set GPIO function for <pin> (input, output, alt...).  settings for other
// pins should be unchanged.
void gpio_set_function(unsigned pin, gpio_func_t function) {
    if(pin >= 32 || function < 0 || function >= 8)
        return;

    uint8_t sel = pin / 10;
    uint8_t idx = pin % 10;
    uint32_t fsel = GPIO_BASE + 4 * sel;

    uint32_t read_val = GET32(fsel);
    uint32_t mask = 0b111 << (3 * idx);
    uint32_t write_val = read_val & ~mask;
    write_val |= function << (3 * idx);
    PUT32(fsel, write_val);
}


//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
    if(pin >= 32)
        return;
    PUT32(gpio_set0, 1 << pin);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
    if(pin >= 32)
        return;
    PUT32(gpio_clr0, 1 << pin);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
    if (pin >= 32) {
        return -1;
    }
    uint32_t val = GET32(gpio_lev0);
    int bit = val & (1 << pin);
    return DEV_VAL32(bit > 0);
}

// Part 3: Pullup or Pulldown

static void gpio_pud_helper(unsigned pin, uint32_t pud) {
    if (pin >= 32)
        return;
    PUT32(gpio_pud, pud);
    delay_cycles(40000);
    PUT32(gpio_pudclk, 1 << pin);
    delay_cycles(40000);
    PUT32(gpio_pudclk, 0);
}

/**
 * Activate the pullup register on GPIO <pin>.
 *
 * GPIO <pin> must be an input pin.
 */
void gpio_set_pullup(unsigned pin) {
    gpio_pud_helper(pin, 0b10);
}

/**
 * Activate the pulldown register on GPIO <pin>.
 *
 * GPIO <pin> must be an input pin.
 */
void gpio_set_pulldown(unsigned pin) {
    gpio_pud_helper(pin, 0b01);
}

/**
 * Deactivate both the pullup and pulldown registers on GPIO <pin>.
 *
 * GPIO <pin> must be an input pin.
 */
void gpio_pud_off(unsigned pin) {
    gpio_pud_helper(pin, 0b00);
}

