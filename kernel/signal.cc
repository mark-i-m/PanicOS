#include "signal.h"
#include "process.h"
#include "machine.h"

////////////////////////////////////////////////////////////////////////////////
// modeled on Linux kernel 3.17.1
////////////////////////////////////////////////////////////////////////////////

typedef struct sigframe {
    jumpercode *returnadr;
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

typedef struct __attribute__((packed)) jumpercode {
    uint8_t movlecx;
    sigframe *frameptr; // pointer to signal frame
    uint8_t movleax;
    uint32_t sigret; // sigret syscall number
    uint16_t int100; // int $0x64
} jumpercode;

static const jumpercode jumpertemplate = {
    0xB9,
    (sigframe*)0x0,
    0xB8,
    0xFF,
    0xCD64
};

// used from linux kernel
#define STACK_ALIGN(esp) ((((esp) + 4) & -16ul) - 4)

sigframe *Signal::getSignalFrame(){
    sigframe *frame;

    frame = (sigframe*)STACK_ALIGN(Process::current->uesp - sizeof(sigframe));

    return frame;
}

jumpercode *Signal::putJumperCode(){
    jumpercode *jumper;

    // get address on stack
    // put it after the sigframe (arbitrary choice)
    jumper = (jumpercode*)STACK_ALIGN(Process::current->uesp - sizeof(sigframe) - sizeof(jumpercode));
    *jumper = jumpertemplate;

    return jumper;
}

// will run in kernel mode, with interrupts disabled
void Signal::checkSignals(SimpleQueue<Signal*> *signals) {
    while(!signals->isEmpty()){
        // wait for this process to enter user mode
        if((uint32_t)Process::current->uesp < 0x80000000) return;
        signals->removeHead()->doSignal();
        Process::current->checkKilled(); // in case a signal killed it
    }
}

// will run in kernel mode, with interrupts disabled
void Signal::doSignal(){
    Process::trace("doing signal %d", sig);

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

    // set up routine to come back to kernel mode
    jumpercode *jumper = putJumperCode();

    // set up stack frame for handler:
    // RA for user code will be set to the above
    // routine
    sigframe *frame = getSignalFrame();

    jumper->frameptr = frame;
    frame->returnadr = jumper;

    // switch to user mode
    switchToUser((uint32_t)Process::current->signalHandler, (uint32_t)frame, 0);
}
