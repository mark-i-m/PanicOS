#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "queue.h"
#include "debug.h"
#include "atomic.h"
#include "semaphore.h"
#include "stdint.h"
#include "process.h"

typedef void (*SignalHandler)(uint32_t);

enum signal_t {
    SIGALRM
};

enum signal_action_t {
    IGNORE, // will not trigger the sig handler
    EXIT // can be handled
};

struct sigframe;
struct jumpercode;

class Signal{
    private:
    signal_t sig;

    public:
    Signal(signal_t sig) : sig(sig) {}

    /* Handle all signals in this process signal queue */
    static void checkSignals(SimpleQueue<Signal> *signals);

    /* Handle this signal */
    void doSignal();
    void setupFrame();
    sigframe *getSignalFrame();
    jumpercode *putJumperCode(sigframe*);
};

#endif
