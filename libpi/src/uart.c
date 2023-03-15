// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

#define AUX_ENABLES 0x20215004
#define UART_DATA 0x20215040
#define UART_INTERRUPT_ENABLE 0x20215044
#define UART_INTERRUPT_IDENTIFY 0x20215048
#define UART_LINE_CONTROL 0x2021504C
#define UART_LINE_STATUS 0x20215054
#define UART_CONTROL 0x20215060
#define UART_STAT 0x20215064
#define UART_BAUD 0x20215068

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    // Config pins for UART
    dev_barrier();
    gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);
    gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
    dev_barrier();

    // Enable mini UART
    PUT32(AUX_ENABLES, GET32(AUX_ENABLES) | 1);
    dev_barrier();

    // Disable TX/RX
    PUT32(UART_CONTROL, 0);
    dev_barrier();

    // while ((GET32(UART_STAT) & 0b100) == 0) {
    // }

    // Disable interrupts
    PUT32(UART_INTERRUPT_ENABLE, 0);
    dev_barrier();
    // Set to 8-bit mode
    PUT32(UART_LINE_CONTROL, 0b11);
    dev_barrier();
    // Clear FIFO queues
    PUT32(UART_INTERRUPT_IDENTIFY, 0b11000111);
    dev_barrier();
    // Set baud rate to 115200
    PUT32(UART_BAUD, 270);
    dev_barrier();

    // Enable
    PUT32(UART_CONTROL, 0b11);
    dev_barrier();
}

// disable the uart.
void uart_disable(void) {
    dev_barrier();
    uart_flush_tx();
    dev_barrier();
    PUT32(AUX_ENABLES, GET32(AUX_ENABLES) & ~(0b1));
    dev_barrier();
}

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    dev_barrier();
    int val = GET32(UART_STAT) & 0b1;
    dev_barrier();
    return val;
}

// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
	while (!uart_has_data()) {
    }
    int ret = GET32(UART_DATA) & 0xFF;
    dev_barrier();
    return ret;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    dev_barrier();
    int val = (GET32(UART_STAT) & 0b10) != 0;
    dev_barrier();
    return val;
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    while (!uart_can_put8()) {
    }
    PUT32(UART_DATA, c);
    dev_barrier();
    return 0;
}

// simple wrapper routines useful later.

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    return (GET32(UART_STAT) & (1 << 9)) != 0;
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty());
}
