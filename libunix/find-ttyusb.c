// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "libunix.h"

#define _SVID_SOURCE

static const char *ttyusb_prefixes[] = {
	"ttyUSB",	// linux
    "ttyACM",   // linux
	"cu.SLAB_USB", // mac os
    "cu.usbserial", // mac os
    // if your system uses another name, add it.
	0
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
    const char** pointer = ttyusb_prefixes;
    while (*pointer != NULL) {
        if (strstr(d->d_name, *pointer) != NULL) {
            return 1;
        }
        pointer++;
    }
    return 0;
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    // use <alphasort> in <scandir>
    // return a malloc'd name so doesn't corrupt.
    // int dirfd = open("/dev", O_DIRECTORY | O_RDONLY);
    // if (dirfd == -1) {
    //     perror("Error in open");
    // }
    struct dirent** dirs;
    int num_dir = scandir("/dev", &dirs, filter, alphasort);
    if (num_dir == -1) {
        panic("scandir failed\n");
    }
    if (num_dir == 0) {
        panic("Found no USB devices connected!\n");
    }
    if (num_dir > 1) {
        panic("Found multiple USB devices connected!\n");
    }
    struct dirent* device = *dirs;
    const static char* prefix = "/dev/";
    char* buf = calloc(1, strlen(device->d_name) + strlen(prefix) + 1);
    sprintf(buf, "%s%s", prefix, device->d_name);
    free(*dirs);
    return buf;
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
    struct dirent** dirs;
    int num_dir = scandir("/dev", &dirs, filter, alphasort);
    if (num_dir == -1) {
        panic("scandir failed\n");
    }
    if (num_dir == 0) {
        panic("Found no USB devices connected!\n");
    }
    
    time_t last_time = 0;
    char* last_usb = NULL;
    
    for (int i = 0; i < num_dir; i++) {
        struct dirent* device = *dirs;
        const static char* prefix = "/dev/";
        char* buf = calloc(1, strlen(device->d_name) + strlen(prefix) + 1);
        sprintf(buf, "%s%s", prefix, device->d_name);
        free(*dirs);
        dirs++;
        
        struct stat file_info;
        int ret = stat(buf, &file_info);
        if (ret == -1) {
            panic("Error in stat\n");
        }
        if (file_info.st_mtime > last_time) {
            last_time = file_info.st_mtime;
            last_usb = buf;
        }
        else {
            free(buf);
        }
    }
    return last_usb;
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
    struct dirent** dirs;
    int num_dir = scandir("/dev", &dirs, filter, alphasort);
    if (num_dir == -1) {
        panic("scandir failed\n");
    }
    if (num_dir == 0) {
        panic("Found no USB devices connected!\n");
    }
    
    time_t first_time = INT32_MAX;
    char* first_usb = NULL;
    
    for (int i = 0; i < num_dir; i++) {
        struct dirent* device = *dirs;
        const static char* prefix = "/dev/";
        char* buf = calloc(1, strlen(device->d_name) + strlen(prefix) + 1);
        sprintf(buf, "%s%s", prefix, device->d_name);
        free(*dirs);
        dirs++;
        
        struct stat file_info;
        int ret = stat(buf, &file_info);
        if (ret == -1) {
            panic("Error in stat\n");
        }
        if (file_info.st_mtime < first_time) {
            first_time = file_info.st_mtime;
            first_usb = buf;
        }
        else {
            free(buf);
        }
    }
    return first_usb;    
}
