#include "libc.h"

int main(int argc, char** argv) {
    for (int i=1; i<argc; i++) {
        int fd = open(argv[i]);
        if (fd < 0) {
            puts(argv[0]); puts(": ");
            puts(argv[i]); puts(": No such file or directory\n");
        } else {
            char buf[100];
            while(1) {
                long n = read(fd,buf,100);
                if (n == 0) break;
                if (n < 0) {
                    puts("error reading : "); puts(argv[i]); puts("\n");
                    break;
                }
                for (int j = 0; j < n; j++) {
                    putchar(buf[j]);
                }
            }
        }
        close(fd);
    }
    return 0;
}
