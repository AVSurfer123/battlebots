/*****************************************************************
 * bootloader implementation.  all the code you write will be in
 * this file.  <get-code.h> has useful helpers.
 *
 * if you need to print: use boot_putk.  only do this when you
 * are *not* expecting any data.
 */
#include "uart_helpers.h"
#include "nrf.h"
#include "nrf-test.h"
#include "joy_def.h"

// wait until:
//   (1) there is data (uart_has_data() == 1): return 1.
//   (2) <timeout> usec expires, return 0.
//
// look at libpi/staff-src/timer.c
//   - call <timer_get_usec()> to get usec
//   - look at <delay_us()> : for how to correctly 
//     wait for <n> microseconds given that the hardware
//     counter can overflow.
static unsigned has_data_timeout(unsigned timeout) {
    uint32_t s = timer_get_usec();
    while (timer_get_usec() - s < timeout) {
        if (uart_has_data()) {
            return 1;
        }
    }
    return 0;
}

nrf_t* server;

void connect_to_joystick() {
    int MAGIC = 0xbadaba;
    while (1) {
        if (boot_get32() != MAGIC) {
            uart_get8();
        }
        else {
            uart_put8(0xcc);
            printk("Received MAGIC value! Sending ack\n");
            break;
        }
    }
    int i = 0;
    while (boot_get32() == MAGIC) {
        i++;
    }
    printk("Cleared %d extra MAGIC in the buffer\n", i);
    boot_get32(); // Read another 32 bits so we finish reading the first joystick struct
}

int led_val = 0;

void read_joystick() {
    axis_state axes[6];
    memset(axes, 0, sizeof(axis_state) * 6);
    gpio_set_output(27);

    // If we don't receive data for 10 seconds, assume connection is closed
    while (has_data_timeout(10000000)) {
        js_event event;
        for (int i = 0; i < sizeof(event); i++) {
            uint8_t* ptr = (uint8_t*) &event;
            ptr[i] = uart_get8();
        }
        size_t axis;
        printk("Read type %d\n", event.type);
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
                break;
        }

        if (event.type == JS_EVENT_BUTTON && event.number == 1 && event.value) {
            led_val = !led_val;
            gpio_write(27, led_val);
        }

        if (event.type == JS_EVENT_BUTTON || event.type == JS_EVENT_AXIS) {
            int ret = nrf_send_noack(server, client_addr, &event, sizeof(js_event));
            if (ret != sizeof(js_event)) {
                printk("nrf send error: only sent %d bytes of %d\n", ret, sizeof(js_event));
            }
        }
        else {
            printk("Read type %d\n", event.type);
        }
    }
}

void notmain(void) {
    server = server_mk_noack(server_addr, sizeof(js_event));

    connect_to_joystick();
    read_joystick();
}
