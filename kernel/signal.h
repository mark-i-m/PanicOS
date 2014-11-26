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

struct __attribute__((packed)) regs {
public:
    uint32_t cr2;
	uint16_t ds;
	uint32_t ebp;
	uint32_t edi;
	uint32_t esi;
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t eax;
	uint32_t eip;
	uint16_t cs;
	uint32_t flags;
	uint32_t esp;
	uint16_t ss;

    regs() {}
};

struct sigframe {
public:
    jumpercode *returnadr;
    uint32_t signal;

    sigframe() {}
};

struct sigcontext {
public:
    regs *registers;
    //uint32_t disableCount; //do we need these?
    //uint32_t iDepth;

    sigcontext(){
        registers = new regs();
    }
    ~sigcontext(){
        delete registers;
    }
};

class Signal{
    private:
    signal_t sig;

    void setupFrame();
    sigframe *getSignalFrame(jumpercode*);
    jumpercode *putJumperCode();

    public:
    Signal(signal_t sig) : sig(sig) {}

    /* Handle all signals in this process signal queue */
    static void checkSignals(SimpleQueue<Signal*> *signals);

    /* Handle this signal */
    void doSignal();
};

#endif
