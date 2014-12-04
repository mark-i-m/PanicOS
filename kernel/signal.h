#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "queue.h"
#include "debug.h"
#include "atomic.h"
#include "semaphore.h"
#include "stdint.h"

enum signal_t {
    SIGINT  = 2,
    SIGALRM = 14,
    SIGSEGV = 11,
    SIGKILL = 9,
    SIGCHLD = 17,
    SIGNUM // ALWAYS the last one, represents the number of signals
};

enum signal_action_t {
    IGNORE, // will not trigger the sig handler
    DEFAULT, // Converted to the default
    EXIT, // kill the process
    HANDLE, // call the signal handler
    NOTFOUND // not a signal number
};

typedef void (*SignalHandler)(uint32_t);

struct jumpercode;

struct __attribute__((packed)) regs {
public:
    uint32_t cr2;
	uint32_t ds;
	uint32_t ebp;
	uint32_t edi;
	uint32_t esi;
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t eax;
	uint32_t eip;
	uint32_t cs;
	uint32_t flags;
	uint32_t esp;
	uint32_t ss;

    regs() :
        cr2(0), ds(0), ebp(0),
        edi(0), esi(0), edx(0),
        ecx(0), ebx(0), eax(0),
        eip(0), cs(0), flags(0),
        esp(0), ss(0) {}
};

struct sigframe {
public:
    jumpercode *returnadr;
    regs *context;
    regs registers;

    sigframe() {}
};

struct sigcontext {
public:
    regs *registers;
    sigframe *frame;

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
    static signal_action_t defaultDisposition(signal_t);
    static void initHandlers(uint32_t (&handlers)[SIGNUM]);
    static bool validateSignal(signal_t s);

    /* Handle this signal */
    void doSignal();
};

#endif
