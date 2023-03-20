// simple software 8n1 UART bit-banging implemention.
//   - look for todo() and implement!
//
// NOTE: using usec is going to be too slow for high baudrate, 
// but should be ok for 115200.
//
#include "rpi.h"
#include "cycle-count.h"
#include "sw-uart.h"
#include "cycle-util.h"

// simple putk to the given <uart>
void sw_uart_putk(sw_uart_t *uart, const char *msg) {
    for(; *msg; msg++)
        sw_uart_put8(uart, *msg);
}

int sw_uart_gets(sw_uart_t *uart, char *in, unsigned nbytes) {
    int read = 0;
    while (read < nbytes) {
        char byte = sw_uart_get8(uart);
        in[read] = byte;
        read++;
        if (byte == '\n') {
            in[read] = 0;
            return read;
        }
    }
    in[read] = 0;
    return read;
}

int sw_uart_get8_timeout(sw_uart_t *uart, uint32_t timeout_usec) {
    return sw_uart_get8(uart);
}

// helper: cleans up the code: do a write for <usec> microseconds
//
// code that uses it will have a lot of compounding error, however.  
// if you switch to using cycles for faster baud rates, you should
// instead use
//      <write_cyc_until> in libpi/include/cycle-util.h
static inline void timed_write(int pin, int v, unsigned cyc) {
    // gpio_write(pin,v);
    // delay_us(usec);
    uint32_t start = cycle_cnt_read();
    write_cyc_until(pin, v, start, cyc);
}

// do this first: used timed_write to cleanup.
//  recall: time to write each bit (0 or 1) is in <uart->usec_per_bit>
void sw_uart_put8(sw_uart_t *uart, unsigned char c) {
    timed_write(uart->tx, 0, uart->cycle_per_bit);
    for (int i = 0; i < 8; i++) {
        timed_write(uart->tx, (c >> i) & 1, uart->cycle_per_bit);
    }
    timed_write(uart->tx, 1, uart->cycle_per_bit);
}

// do this second: you can type in pi-cat to send stuff.
//      EASY BUG: if you are reading input, but you do not get here in 
//      time it will disappear.
int sw_uart_get8(sw_uart_t *uart) {
    while (gpio_read(uart->rx) == 1) {
    }
    uint32_t start = cycle_cnt_read();
    int byte = 0;
    uint32_t half = uart->cycle_per_bit / 2;
    for (int i = 0; i < 8; i++) {
        delay_ncycles(start, (i + 1) * uart->cycle_per_bit + half);
        byte |= gpio_read(uart->rx) << i;
    }
    delay_ncycles(start, 9 * uart->cycle_per_bit + half);
    assert(gpio_read(uart->rx) == 1); // Stop bit should be set now
    return byte;
}

// setup the GPIO pins
sw_uart_t sw_uart_mk_helper(unsigned tx, unsigned rx,
        unsigned baud,
        unsigned cyc_per_bit,
        unsigned usec_per_bit) {

    // implement:
    //  1. set rx and tx as input/output.
    //  2: what is the default value of tx for 8n1?  make sure
    //     this is set!!
    // todo("set up the right GPIO pins with the right values");
    gpio_set_input(rx);
    gpio_set_pullup(rx);
    gpio_set_output(tx);
    timed_write(tx, 1, cyc_per_bit);

    // check that the given values make sense.
    //
    // we give you the rest.
    // make sure the value makes sense.
    unsigned mhz = 700 * 1000 * 1000;
    unsigned derived = cyc_per_bit * baud;
    assert((mhz - baud) <= derived && derived <= (mhz + baud));
    // panic("cyc_per_bit = %d * baud = %d\n", cyc_per_bit, cyc_per_bit * baud);

    return (sw_uart_t) {
            .tx = tx,
            .rx = rx,
            .baud = baud,
            .cycle_per_bit = cyc_per_bit ,
            .usec_per_bit = usec_per_bit
    };
}
