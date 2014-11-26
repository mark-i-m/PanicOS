#include "signal.h"
#include "process.h"
#include "machine.h"
#include "pic.h"

////////////////////////////////////////////////////////////////////////////////
// modeled on Linux kernel 3.17.1
////////////////////////////////////////////////////////////////////////////////

// assembly jumper function that calls sys_sigret
typedef struct __attribute__((packed)) jumpercode {
    uint8_t movleax;
    uint32_t sigret; // sigret syscall #
    uint8_t interrupt; // int $0x64
    uint32_t syscall; // interupt #
} jumpercode;

static const jumpercode jumpertemplate = {
    0xB8,
    0xFF,
    0xCD,
    0x64
};

// used from linux kernel
#define STACK_ALIGN(esp) ((((esp) + 4) & -16ul) - 4)

sigframe *Signal::getSignalFrame(jumpercode *jumper){
    sigframe *frame = 0;
    //Process *me = Process::current;

    //frame = (sigframe*)STACK_ALIGN(me->context->registers->esp - sizeof(sigframe));
    //frame->returnadr = jumper;
    //frame->signal = sig;

    return frame;
}

jumpercode *Signal::putJumperCode(){
    jumpercode *jumper = 0;
    //Process *me = Process::current;

    // get address on stack
    // put it after the sigframe (arbitrary choice)
    //jumper = (jumpercode*)STACK_ALIGN(me->context->registers->esp
                         //- sizeof(sigframe) - sizeof(jumpercode));
    //*jumper = jumpertemplate;

    return jumper;
}

// will run in kernel mode, with interrupts disabled
void Signal::checkSignals(SimpleQueue<Signal*> *signals) {

    // wait for this process to enter user mode
    //if((uint32_t)Process::current->context->registers->esp < 0x80000000) return;

    //while(1){
    //    bool isEmpty;

    //    // this is the only place where items are removed
    //    // from the queue, so we don't have to worry about
    //    // race conditions between this lock and the next
//  //      Process::current->signalMutex->lock();
    //    isEmpty = signals->isEmpty();
//  //      Process::current->signalMutex->unlock();

    //    if(isEmpty) return;

    //    // remove the head of the list
//  //      Process::current->signalMutex->lock();
    //    Signal *sig = signals->removeHead();
//  //      Process::current->signalMutex->unlock();

    //    // handle the signal
    //    sig->doSignal();
    //    Process::current->checkKilled(); // in case a signal killed it
    //}
}

// will run in kernel mode, with interrupts disabled
void Signal::doSignal(){
    //Process::trace("doing signal %d", sig);

    ////find out what the action for this signal should be
    //signal_action_t action = Process::current->getSignalAction(sig);

    //switch(action) {
    //    case IGNORE:
    //        return;
    //    case EXIT:
    //        // if there is a signal handler
    //        if (Process::current->signalHandler) {
    //            setupFrame();
    //        } else {
    //            // kill the process with the signal code
    //            Process::current->kill(sig);
    //        }
    //        return;
    //}
}

void Signal::setupFrame(){

    // set up jumper code to call sys_sigret
    //jumpercode *jumper = putJumperCode();

    //// set up stack frame for handler.
    //// we must return to the jumper code
    //sigframe *frame = getSignalFrame(jumper);

    //// enable interupts during the handler
    //Process::endIrq();

    //switchToUser((uint32_t)Process::current->signalHandler, (uint32_t)frame, 0);
}
