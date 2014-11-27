#include "libc.h"

volatile unsigned short numSignals = 0;
volatile unsigned char isSignaled = 0;

int main();
void contextTest();

void sigtestHandler(regs *context) {
    puts("YAYAYAY!!\n");
    context->eip = (unsigned long)&contextTest;
    isSignaled = 1;
    return;
}

void contextTest(){
    exit(0xCAFE);
}

void alarmHandler(long context) {
    numSignals++;
    puts("shutdown in T-");
    putdec(10 - numSignals);
    puts(" seconds!\n");
    return;
};

int main(){
    puts("in test\n");

    long s = semaphore(0);
    long fk = fork();
    if(fk == 0){
        signal(SIGTEST, (void*)&sigtestHandler);
        up(s);
        while(!isSignaled);
        exit(0xBEEF);
    } else {
        down(s); // wait until the child has registered the signal handler
        kill(fk, 0);
        long ret = join(fk);
        puts("child exited with code = 0x");
        puthex(ret);
        puts("\n");
    }

    puts("counting down to shutdown\n");

    fk = fork();
    if(fk == 0) {
        signal(SIGALRM, (void*)&alarmHandler);
        alarm(1);
        while(numSignals < 10);
        exit(0xCAFE);
    } else {
        long ret = join(fk);
        puts("child exited with code = 0x");
        puthex(ret);
        puts("\n");
        shutdown();
    }
    return 0;
}
