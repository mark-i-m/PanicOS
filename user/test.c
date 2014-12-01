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
    //puts("contextTest\n");
    exit(0xCAFE);
}

void alarmHandler(long context) {
    numSignals++;
    puts("shutdown in T-");
    putdec(10 - numSignals);
    puts(" seconds!\n");
    alarm(1);
    return;
};

void sigchldHandler(regs *context) {
    puts("child exited and signaled\n");
    //puts("esp=");
    //puthex(context->esp);
    //puts("\n");
    //puts("pc=");
    //puthex(context->eip);
    //puts("\n");
    isSignaled = 1;
}

//void handleSegfault(long context) {
//    puts("handled SIGSEGV!!\n");
//}

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
        kill(fk, SIGTEST);
        long ret = join(fk);
        puts("child exited with code = 0x");
        puthex(ret);
        puts("\n");
    }

    fk = fork();
    if(fk == 0){
        down(s); // wait for parent to register handler
        //puts("exiting\n");
        exit(0xCAFE);
    } else {
        // wait for child to die
        signal(SIGCHLD, (void*)&sigchldHandler);
        up(s);
        // wait for signal
        while(!isSignaled){
            //puts("X");
        }
    }

    //puts("handle SIGSEGV\n");
    //signal(SIGSEGV, &handleSegfault);

    //*((unsigned int*)0) = 0xFACEBEEF;

    puts("counting down to shutdown\n");

    signal(SIGCHLD, SIG_IGN);

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
