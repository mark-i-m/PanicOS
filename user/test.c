#include "libc.h"

void sigHandler(long sig) {
    switch(sig){
        case 0:
            puts("YAYAYAY!!\n");
            return;
        default:
            puts("Unknown signal: ");
            putdec(sig);
            puts("\n");
            return;
    }
};

int main(){
    puts("in test\n");

    long s = semaphore(0);
    long fk = fork();
    if(fk == 0){
        handler((void*)&sigHandler);
        up(s);
        int i;
        for(i = 0; i < 10000000; i++) {}
        exit(0xCAFE);
    } else {
        down(s); // wait until the child has registered the signal handler
        signal(fk, 0);
        long ret = join(fk);
        puts("child exited with code = 0x");
        puthex(ret);
        puts("\n");
        shutdown();
    }
    return 0;
}
