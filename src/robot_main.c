#include "rpi.h"
#include "nrf.h"
#include "nrf-test.h"
#include "joy_def.h"

nrf_t* client;

axis_state axes[6];

void notmain() {
    client = client_mk_noack(client_addr, sizeof(js_event));

    js_event event;
    size_t axis;
    int total_wait = 0;

    while (1) {
        int ret = nrf_read_exact_noblk(client, &event, sizeof(js_event));
        if (ret != sizeof(js_event)) {
            delay_ms(1);
            total_wait++;
        }
        else {
            total_wait = 0;
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
                    printk("Read type %d\n", event.type);
                    break;
            }
        }
        if (total_wait > 10000) {
            clean_reboot();
        }
    }
}