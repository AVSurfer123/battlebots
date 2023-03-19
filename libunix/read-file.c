#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.   
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s. 
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
// 
void *read_file(unsigned *size, const char *name) {
    // How: 
    //    - use stat to get the size of the file.
    //    - round up to a multiple of 4.
    //    - allocate a buffer
    //    - zero pads to a multiple of 4.
    //    - read entire file into buffer.  
    //    - make sure any padding bytes have zeros.
    //    - return it.   
    struct stat file_info;
    int ret = stat(name, &file_info);
    if (ret == -1) {
        perror("Error in stat");
    }
    int mod = file_info.st_size % 4;
    int pad = mod == 0 ? 0 : 4 - mod;
    int buf_size = file_info.st_size + pad;
    void* buf = calloc(1, buf_size);
    if (buf == NULL) {
        perror("calloc failed");
    }

    int fd = open(name, O_RDONLY);
    if (fd == -1) {
        perror("Error in open");
    }
    int num_read = read_exact(fd, buf, file_info.st_size);
    close(fd);
    *size = num_read;
    return buf;
}
