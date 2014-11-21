#include "libc.h"

void notFound(char* cmd) {
    puts(cmd);
    puts(": command not found\n");
}

int main() {
    while (1) {
        puts("shell> ");
        char* in = gets();
        puts(in);
        puts("\n");
        if (in) free(in);
    }
    return 0;
}
