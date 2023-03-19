/*****************************************************************
 * bootloader implementation.  all the code you write will be in
 * this file.  <get-code.h> has useful helpers.
 *
 * if you need to print: use boot_putk.  only do this when you
 * are *not* expecting any data.
 */
#include "uart_helpers.h"

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

#define JS_EVENT_BUTTON		0x01	/* button pressed/released */
#define JS_EVENT_AXIS		0x02	/* joystick moved */
#define JS_EVENT_INIT		0x80	/* initial state of device */

struct js_event {
	uint32_t time;	/* event timestamp in milliseconds */
	int16_t value;	/* value */
	uint8_t type;	/* event type */
	uint8_t number;	/* axis/button number */
};

/**
 * Current state of an axis.
 */
struct axis_state {
    int16_t x, y;
};

/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state* axes)
{
    size_t axis = event->number / 2;

    if (event->number % 2 == 0)
        axes[axis].x = event->value;
    else
        axes[axis].y = event->value;

    return axis;
}


void connect_to_joystick() {
    int MAGIC = 0xbadaba;
    while (1) {
        if (uart_get32() != MAGIC) {
            uart_get8();
        }
        else {
            uart_put8(0xcc);
            printk("Received MAGIC value! Sending ack\n");
            break;
        }
    }
    int i = 0;
    while (uart_get32() == MAGIC) {
        i++;
    }
    printk("Cleared %d extra MAGIC in the buffer\n", i);
    uart_get32(); // Read another 32 bits so we finish reading the first joystick struct
}

void read_joystick() {
    struct axis_state axes[6];

    // If we don't receive data for 1 second, assume connection is closed
    while (has_data_timeout(1000000)) {
        printk("Got data!\n");

        struct js_event event;
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
    }
}

void notmain(void) {
    while (1) {
        connect_to_joystick();
        read_joystick();
    }
}
