#include "libc.h"

int main(){
    puts("in test\n");

    long fk = fork();
    if(fk == 0){
        while(1);
    } else {
        signal(fk, 0);
    }
    return 0;
}
