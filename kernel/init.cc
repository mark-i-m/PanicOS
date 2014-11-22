#include "init.h"
#include "elf.h"
#include "machine.h"
#include "fs.h"
#include "libk.h"

Init::Init() : Process("init",nullptr) {
}

long Init::run() {

    trace("SIGALRM");
    Process::current->signal(SIGALRM);

    while(1) {
        trace("looping");      
    }

    Debug::shutdown("What?");
    return 0;
}
