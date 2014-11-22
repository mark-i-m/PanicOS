#include "libc.h"

void sigHandler(long sig) {
    switch(sig){
        case 0:
            puts("YAYAYAY!!");
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

    long fk = fork();
    if(fk == 0){
        handler((void*)&sigHandler);
        while(1);
    } else {
        signal(fk, 0);
    }
    return 0;
}
