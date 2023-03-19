#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>

#include "libunix.h"

/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
    __u8 axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}

/**
 * Current state of an axis.
 */
struct axis_state {
    short x, y;
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

void connect_to_pi(int fd) {
    while (1) {
        int MAGIC = 0xbadaba;
        put_uint32(fd, MAGIC);
        usleep(1);
        uint8_t end = 0;
        if (read(fd, &end, 1) == 1) {
            printf("Got a byte from the Pi\n");
            if (end == 0xcc) {
                printf("Pi is connected! sending joystick info now\n");
            }
            else {
                panic("Didn't get correct byte. something is wrong...\n");
            }
            break;
        }
    }
    usleep(1);
}

void send_joystick(int fd, int js) {
    struct js_event event;

    // read and echo the characters from the usbtty until it closes 
    // (pi rebooted) or we see a string indicating a clean shutdown.
    while(read_event(js, &event) == 0) {
        printf("reading from joystick: %d\n", event.type);

        // Write joystick data to Pi
        for (int i = 0; i < sizeof(event); i++) {
            uint8_t* ptr = (uint8_t*) &event;
            put_uint8(fd, ptr[i]);
        }
        usleep(10000);

        // Handle data coming from Pi
        static uint8_t buf[4096];
        int n = read(fd, buf, sizeof buf - 1);
        if(n == 0) {
            usleep(1000);
        } else if(n < 0) {
            sys_die(read, "pi connection closed.  cleaning up\n");
        } else {
            buf[n] = 0;
            // if you keep getting "" "" "" it's b/c of the GET_CODE message from bootloader
            remove_nonprint(buf, n);
            printf("%s", buf);
        }
    }
    printf("Joystick disconnected! Ending process\n");
}


int trace_p = 0; 
static char *progname = 0;

static void usage(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    output("\nusage: %s  ([--last] | [--first] | [--device <device>] | [--joy <joy_device>]) \n", progname);
    output("    specify a device using any method:\n");
    output("        --last: gets the last serial device mounted\n");
    output("        --first: gets the first serial device mounted\n");
    output("        --device <device>: manually specify <device>\n");
    output("        --joy <joy_device>: manually specify joystick\n");
    output("    --baud <baud_rate>: manually specify baud_rate. Default is 115200\n");
    exit(1);
}

int main(int argc, char *argv[]) { 
    char *pi_dev = 0;
    char* joy_dev = "/dev/input/js0";

    // a good extension challenge: tune timeout and baud rate transmission
    //
    // on linux, baud rates are defined in:
    //  /usr/include/asm-generic/termbits.h
    //
    // when I tried with sw-uart, these all worked:
    //      B9600
    //      B115200
    //      B230400
    //      B460800
    //      B576000
    // can almost do: just a weird start character.
    //      B1000000
    // all garbage?
    //      B921600
    unsigned baud_rate = B115200;

    // we do manual option parsing to make things a bit more obvious.
    // you might rewrite using getopt().
    progname = argv[0];
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "--last") == 0)  {
            pi_dev = find_ttyusb_last();
        } else if(strcmp(argv[i], "--first") == 0)  {
            pi_dev = find_ttyusb_first();
        // we assume: anything that begins with /dev/ is a device.
        } else if(prefix_cmp(argv[i], "/dev/")) {
            pi_dev = argv[i];
        } else if(strcmp(argv[i], "--baud") == 0) {
            i++;
            if(!argv[i])
                usage("missing argument to --baud\n");
            baud_rate = atoi(argv[i]);
        } else if(strcmp(argv[i], "--device") == 0) {
            i++;
            if(!argv[i])
                usage("missing argument to --device\n");
            pi_dev = argv[i];
        } else {
            usage("unexpected argument=<%s>\n", argv[i]);
        }
    }

    // get the name of the ttyUSB.
    if(!pi_dev) {
        pi_dev = find_ttyusb_last();
        if(!pi_dev)
            panic("didn't find a device\n");
    }
    printf("done with options: dev name=<%s> joy name=<%s>\n", pi_dev, joy_dev);

    // open the joystick
    int js = open(joy_dev, O_RDONLY);
    if (js == -1)
        panic(" Could not open joystick %s\n", joy_dev);
    printf("Joystick info for %s: %lu axes and %lu buttons\n", joy_dev, get_axis_count(js), get_button_count(js));

    // open the ttyUSB
    int tty = open_tty(pi_dev);
    if(tty < 0)
        panic("can't open tty <%s>\n", pi_dev);

    // timeout is in tenths of a second. 0 means no timeout so non-blocking ops
    int fd = set_tty_to_8n1(tty, baud_rate, 0);
    if(fd < 0)
        panic("could not set tty: <%s>\n", pi_dev);

    printf("Got fd from tty: %d\n", fd);

    connect_to_pi(fd);
    send_joystick(fd, js);

    return 0;
}
