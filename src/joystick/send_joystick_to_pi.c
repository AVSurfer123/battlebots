#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>

#include "libunix.h"

// Global variables holding file descriptors to be used between threads
int pi_fd;
int joy_fd;

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

void connect_to_pi() {
    char val;
    int ret = read(pi_fd, &val, 1);
    if (ret == 1) {
        printf("Somehow got something from pi: %c\n", val);
    }
    while (1) {
        int MAGIC = 0xbadaba;
        put_uint32(pi_fd, MAGIC);
        usleep(1);
        uint8_t end = 0;
        if (read(pi_fd, &end, 1) == 1) {
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
    usleep(1000000);
}

void* send_joystick(void* arg) {
    struct js_event event;

    // read and echo the characters from the usbtty until it closes 
    // (pi rebooted) or we see a string indicating a clean shutdown.
    while(read_event(joy_fd, &event) == 0) {
        // printf("reading from joystick: %d\n", event.type);

        // Write joystick data to Pi
        for (int i = 0; i < sizeof(event); i++) {
            uint8_t* ptr = (uint8_t*) &event;
            put_uint8(pi_fd, ptr[i]);
        }
        // Write at 50 Hz
        usleep(20000);
    }
    printf("Joystick disconnected! Ending process\n");
    return NULL;
}

void* read_pi_output(void* arg) {
    uint8_t buf[4096];
    while (1) {
        // Handle data coming from Pi
        int n = read(pi_fd, buf, sizeof buf - 1);
        if(n == 0) {
            usleep(1000);
        } else if(n < 0) {
            sys_die(read, "pi connection closed.  cleaning up\n");
        } else {
            buf[n] = 0;
            // if you keep getting "" "" "" it's b/c of the GET_CODE message from bootloader
            remove_nonprint(buf, n);
            printf("%s", buf);
            if(pi_done(buf)) {
                clean_exit("\npi exited.  cleaning up\n");
            }
        }
    }
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
    exit(1);
}

int main(int argc, char *argv[]) { 
    char *pi_dev = 0;
    char* joy_dev = "/dev/input/js0";

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
    sleep(1);

    // open the joystick
    joy_fd = open(joy_dev, O_RDONLY);
    if (joy_fd == -1)
        panic(" Could not open joystick %s\n", joy_dev);

    // open the ttyUSB
    int tty = open_tty(pi_dev);
    if(tty < 0)
        panic("can't open tty <%s>\n", pi_dev);

    // timeout is in tenths of a second. 0 means no timeout so non-blocking ops
    pi_fd = set_tty_to_8n1(tty, baud_rate, 0);
    if(pi_fd < 0)
        panic("could not set tty: <%s>\n", pi_dev);

    // Connect to pi using magic word
    connect_to_pi();

    pthread_t send_thread, receive_thread;
    int ret = pthread_create(&send_thread, NULL, send_joystick, NULL);
    if (ret != 0) {
        panic("failed to create send thread\n");
    }
    ret = pthread_create(&receive_thread, NULL, read_pi_output, NULL);
    if (ret != 0) {
        panic("failed to create receive thread\n");
    }
    pthread_join(send_thread, NULL);
    // Don't join on receive thread as it won't die if pi is alive

    return 0;
}
