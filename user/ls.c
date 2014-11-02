#include "libc.h"

int main() {
    long fd = open(".");
    if (fd < 0) {
        exit(fd);
    }
    long n = getlen(fd) / 16;
    for (long i = 0; i<n; i++) {
        char buf[13];
        seek(fd,i*16);
        readFully(fd,buf,12); 
        buf[12] = 0;
        puts(buf);
        puts("\n");
    }
    return 0;
}
