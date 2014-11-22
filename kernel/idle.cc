#include "idle.h"
#include "err.h"

long IdleProcess::run(void) {
    while (true) {
        Process::checkReaper();
//        trace("idle");
//        Debug::shutdown("idle");
        __asm__ __volatile__ ("hlt");
    }
    return ERR_NOT_POSSIBLE;
}
