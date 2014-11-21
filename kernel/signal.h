#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "queue.h"
#include "debug.h"
#include "atomic.h"
#include "semaphore.h"
#include "stdint.h"

typedef void (*SignalHandler)(uint32_t);

class Signal {
    public:
        uint32_t num; // signal number

        Signal() {}
};

#endif
