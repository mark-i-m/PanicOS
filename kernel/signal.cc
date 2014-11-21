#include "signal.h"

////////////////////////////////////////////////////////////////////////////////
// modeled on Linux kernel 3.17.1
////////////////////////////////////////////////////////////////////////////////

typedef struct sigframe {
    uint32_t returnadr;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t ds;
	uint32_t eip;
	uint32_t cs;
	uint32_t flags;
	uint32_t esp;
	uint32_t ss;
} sigframe;

typedef struct jumpercode {
    char movlecx = 0xB9;
    sigframe *frameptr; // pointer to signal frame
    char movleax = 0xB8;
    uint32_t sigret = 0xFF; // sigret syscall number
    uint16_t int100 = 0xCD64; // int $0x64
} jumpercode;

sigframe *Signal::getSignalFrame(){
    sigframe *frame;

    MISSING();

    return frame;
}

jumpercode *Signal::putJumperCode(sigframe *frame){
    jumpercode *jumper;

    MISSING();

    jumper->frameptr = frame;

    return jumper;
}

// will run in kernel mode, with interrupts disabled
void Signal::checkSignals(SimpleQueue<Signal> *signals) {
    while(!signals->isEmpty()){
        signals->removeHead().doSignal();

        Process::current->checkKilled(); // in case a signal killed it
    }
}

// will run in kernel mode, with interrupts disabled
void Signal::doSignal(){
    //find out what the action for this signal should be
    signal_action_t action = Process::current->getSignalAction(sig);

    switch(action) {
        case IGNORE:
            return;
        case EXIT:
            // if there is a signal handler
            if (Process::current->signalHandler) {
                setupFrame();
            } else {
                // kill the process with the signal code
                Process::current->kill(sig);
            }

            return;
    }
}

void Signal::setupFrame(){

    // set up stack frame for handler:
    // RA for user code will be set to the above
    // routine
    sigframe *frame = getSignalFrame();

    // set up routine to come back to kernel mode
    jumpercode *jumper = putJumperCode(frame);

    // switch to user mode
    MISSING();
}
