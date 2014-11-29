#include "signal.h"
#include "process.h"
#include "machine.h"
#include "pic.h"

////////////////////////////////////////////////////////////////////////////////
// modeled on Linux kernel 3.17.1
////////////////////////////////////////////////////////////////////////////////

// assembly jumper function that calls sys_sigret
struct __attribute__((packed)) jumpercode {
public:
    uint8_t movleax;
    uint32_t sigret; // sigret syscall #
    uint8_t interrupt; // int $0x64
    uint32_t syscall; // interupt #

    jumpercode() :
        movleax(0xB8),
        sigret(0xFF),
        interrupt(0xCD),
        syscall(0x64) {}
};

// used from linux kernel
#define STACK_ALIGN(esp) ((esp) >> 4 << 4)

sigframe *Signal::getSignalFrame(jumpercode *jumper){
    sigframe *frame;
    Process *me = Process::current;

    frame = (sigframe*)STACK_ALIGN(me->context->registers->esp - sizeof(sigframe));
    frame->returnadr = jumper;
    frame->context = &frame->registers;
    frame->registers = *me->context->registers;
    me->context->frame = frame;

    return frame;
}

jumpercode *Signal::putJumperCode(){
    jumpercode *jumper;
    Process *me = Process::current;

    // get address on stack
    // put it after the sigframe (arbitrary choice)
    jumper = (jumpercode*)STACK_ALIGN(me->context->registers->esp
                       - sizeof(sigframe) - sizeof(jumpercode));
    *jumper = jumpercode();

    return jumper;
}

// will run in kernel mode, with interrupts disabled
void Signal::checkSignals(SimpleQueue<Signal*> *signals) {

    // wait for this process to enter user mode
    //Debug::printf("uesp=%X\n",Process::current->context->registers->esp);
    if((uint32_t)Process::current->context->registers->esp < 0x80000000) return;

    while(1){
        bool isEmpty;

        // this is the only place where items are removed
        // from the queue, so we don't have to worry about
        // race conditions between this lock and the next
//        Process::current->signalMutex->lock();
        isEmpty = signals->isEmpty();
//        Process::current->signalMutex->unlock();

        if(isEmpty) return;

        // remove the head of the list
//        Process::current->signalMutex->lock();
        Signal *sig = signals->removeHead();
//        Process::current->signalMutex->unlock();

        // handle the signal
        sig->doSignal();
        Process::current->checkKilled(); // in case a signal killed it
    }
}

// will run in kernel mode, with interrupts disabled
void Signal::doSignal(){
    //Process::trace("doing signal %d", sig);

    //find out what the action for this signal should be
    signal_action_t action = Process::current->getSignalAction(sig);

    switch(action) {
        case IGNORE:
            return;
        case EXIT:
            // kill the process with the signal code
            Process::current->kill(sig);
            return;
        case HANDLE:
            // handle the signal
            setupFrame();
            return;
        default: // should never happen
            return;
    }
}

void Signal::setupFrame(){
    // set up jumper code to call sys_sigret
    jumpercode *jumper = putJumperCode();

    // set up stack frame for handler.
    // we must return to the jumper code
    sigframe *frame = getSignalFrame(jumper);

    // enable interupts during the handler
    Process::current->iDepth --;
    Process::current->disableCount = 0;

//    Debug::printf("going to jmp to %x\n", Process::current->signalHandlers[sig]);

    switchToUser((uint32_t)Process::current->signalHandlers[sig], (uint32_t)frame, 0);
}

signal_action_t Signal::defaultDisposition(signal_t sig) {
    switch(sig) {
        case SIGTEST: return EXIT;
        case SIGALRM: return IGNORE;
        case SIGSEGV: return EXIT;
        case SIGCHLD: return IGNORE;
        default: return EXIT;
    }
}

void Signal::initHandlers(uint32_t (&handlers)[SIGNUM]) {
    for(int i = 0; i < SIGNUM; i++) {
        handlers[i] = (uint32_t)defaultDisposition((signal_t)i);
    }
}
