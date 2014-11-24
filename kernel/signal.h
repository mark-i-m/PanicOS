#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "queue.h"
#include "debug.h"
#include "atomic.h"
#include "semaphore.h"
#include "stdint.h"

typedef void (*SignalHandler)(uint32_t);

enum signal_t {
    SIGTEST,
    SIGALRM
};

enum signal_action_t {
    IGNORE, // will not trigger the sig handler
    EXIT // can be handled
};

struct jumpercode;

typedef struct sigframe {
    jumpercode *returnadr;
    uint32_t signal;
    uint32_t padding;
	//uint32_t ss;
	uint32_t esp;
	uint32_t flags;
	//uint32_t cs;
	uint32_t eip;
	//uint32_t ds;
	uint32_t ebp;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebx;
    uint32_t disableCount;
    uint32_t iDepth;
} sigframe;

class Signal{
    private:
    signal_t sig;

    public:
    Signal(signal_t sig) : sig(sig) {}

    /* Handle all signals in this process signal queue */
    static void checkSignals(SimpleQueue<Signal*> *signals);

    /* Handle this signal */
    void doSignal();
    void setupFrame();
    sigframe *getSignalFrame();
    jumpercode *putJumperCode();
};

#endif
