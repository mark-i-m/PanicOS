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

    //Process::trace("checking %s#%d's signal queue %x, %x", Process::current->name, Process::current->id, Process::current->signalQueue, signals);

    if(!signals->isEmpty()){
        signals->removeHead()->doSignal();
        Process::current->checkKilled(); // in case a signal killed it
    }
}

// will run in kernel mode, with interrupts disabled
void Signal::doSignal(){
    //find out what the action for this signal should be
    signal_action_t action = Process::current->getSignalAction(sig);

    switch(action) {
        case IGNORE:
            // Process::trace("ignore");
            return;
        case EXIT:
            // Process::trace("kill");
            // kill the process with the signal code
            // enable interupts
            Process::current->kill(sig);
            return;
        case HANDLE:
            //Debug::printf("%s#%d %X doing signal %d\n", Process::current->name, Process::current->id, Process::current, sig);
            // Process::trace("HANDLE");
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

    // Debug::printf("going to jmp to %x\n", Process::current->signalHandlers[sig]);

    switchToUser((uint32_t)Process::current->signalHandlers[sig], (uint32_t)frame, 0);
}

signal_action_t Signal::defaultDisposition(signal_t sig) {
    switch(sig) {
        case SIGINT: return EXIT;
        case SIGALRM: return EXIT;
        case SIGSEGV: return EXIT;
        case SIGCHLD: return IGNORE;
        case SIGKILL: return EXIT;
        default: return NOTFOUND;
    }
}

void Signal::initHandlers(uint32_t (&handlers)[SIGNUM]) {
    for(int i = 0; i < SIGNUM; i++) {
        handlers[i] = (uint32_t)defaultDisposition((signal_t)i);
    }
}

bool Signal::validateSignal(signal_t s) {
    return defaultDisposition(s) != NOTFOUND;
}
