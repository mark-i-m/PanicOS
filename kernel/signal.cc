#include "signal.h"

// will run in kernel mode, with interrupts disabled
void Signal::checkSignals(SimpleQueue<Signal> *signals) {
    while(!signals->isEmpty()){
        signals->removeHead().doSignal();
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
                // set up routine to come back to kernel mode

                // set up stack frame for handler:
                // RA for user code will be set to the above
                // routine, which will return at the end of this
                // method

                // switch to user mode
            } else {
                // kill the process with the signal code
                Process::current->kill(sig);
            }
            return;
    }

    Process::current->checkKilled(); // check again, in case a signal killed it
}
