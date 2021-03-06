#include "process.h"
#include "machine.h"
#include "debug.h"
#include "pic.h"
#include "pit.h"
#include "semaphore.h"
#include "idle.h"
#include "tss.h"
#include "elf.h"
#include "fs.h"
#include "err.h"
#include "libk.h"

/* global process declarations */
Debug* Process::DEBUG;                          // the debug channel
size_t Process::STACK_LONGS = 1024 * 2;         // default kernel stack size
SimpleQueue<Process*> *Process::readyQueue;     // the ready queue
SimpleQueue<Process*> *Process::reaperQueue;    // the reaper queue
Process* Process::current;                      // the current process
Atomic32 Process::nextId;                       // next process ID
Semaphore *Process::traceMutex;
Timer* Process::timers = nullptr;               // pending timers
Alarm *Process::alarms = nullptr;               // pending alarms
Process* Process::idleProcess = nullptr;        // the idle process
uint32_t Process::idleJiffies = 0;              // idle jiffies

void Process::init() {
    DEBUG = new Debug("Process");
    readyQueue = new SimpleQueue<Process*>();
    reaperQueue = new SimpleQueue<Process*>();
    traceMutex = new Semaphore(1);
}

void Process::checkReaper() {
    Process *kill;
    do {
        kill = nullptr;
        Process::disable();
        if (!Process::reaperQueue->isEmpty()) {
            kill = Process::reaperQueue->removeHead();
        }
        Process::enable();
        Resource::unref(kill);
    } while (kill != nullptr);
}

void Process::checkKilled() {
    checkReaper();
    Process *me = Process::current;
    if (me) {
        if (me->isKilled) {
            me->onKill();
            Process::exit(me->killCode);
        }
    }
}

void Process::entry() {
    checkKilled();
    long code = run();
    exit(code);
}

extern "C" void runProcess() {
    Process::current->entry();
}

/* A bit of room to detect stack overflow without
   corrupting neighbors */
#define FUDGE 128

Process::Process(const char* name, Table *resources_) :
    Resource(ResourceType::PROCESS), name(name), parent(nullptr), resources(resources_)
{
    //Debug::printf("Process::Process %p\n",this);
    id = nextId.getThenAdd(1);
    iDepth = 0;
    iCount = 0;
    isKilled = false;
    killCode = 0;
    disableCount = 0;
    stack = new long[FUDGE + STACK_LONGS];
    if (stack == 0) {
        Debug::panic("can't allocate stack");
    }
    stack = &stack[FUDGE];
    //Debug::printf("stack=%X\n",stack);
    int idx = STACK_LONGS;
    stack[--idx] = 0;
    stack[--idx] = (long) runProcess;
    stack[--idx] = 0; /* %ebx */
    stack[--idx] = 0; /* %esi */
    stack[--idx] = 0; /* edi */
    stack[--idx] = 0; /* ebp */
    kesp = (long) &stack[idx];

    /* Resource table */
    if (resources == nullptr) {
        resources = new Table(100);
    }
    Resource::ref(resources);

    signalMutex = new Mutex();

    signalMutex->lock();
    signalQueue = new SimpleQueue<Signal*>();
    context = new sigcontext();
    Signal::initHandlers(signalHandlers);
    signalMutex->unlock();

    inSignal = false;

    /* We always start with a refcount of 1 */
    count.set(1);
}

Process::~Process() {
    delete context;
    delete signalQueue;
    delete signalMutex;

    if (stack) {
        delete[] (stack - FUDGE);
        stack = 0;
    }
    if (resources) {
        Resource::unref(resources);
        resources = nullptr;
    }
}

void Process::start() {
    makeReady();
}

void Process::kill(long code) {

    Process::disable();
    if (!isKilled) {
        isKilled = true;
        killCode = code;
    }
    Process::enable();
    checkKilled();
}

long Process::execv(const char* fileName, SimpleQueue<const char*> *args, long argc) {
    File *prog = FileSystem::rootfs->rootdir->lookupFile(fileName);
    if (prog == nullptr) {
        return ERR_NOT_FOUND;
    }

    name = K::strdup(fileName);

    /* Prepare address space for exec */
    addressSpace.exec();

    /* copy args */

    /*
     *               0                          <- ESP
     *               argv
     *               argc
     *               0
     *               ...
     * argv[argc-1]:
     *               ...
     * argv[argc-2]:
     *               ...
     *
     * ....
     * argv[0]:
     *               0
     *               argv[n-1]
     *               ...
     * 0xffffff0:    argv[0]
     *
     */

    char* bottom = (char*) 0xfffffff0;
    long* userArgs = (long*)(bottom - ((argc + 1) * 4));
    userArgs[argc] = 0;

    /* copy args to user memory */
    char* ptr = (char*) userArgs;
    for (long i=0; i<argc; i++) {
        const char* s = args->removeHead();
        long len = K::strlen(s) + 1;
        ptr -= len;
        userArgs[i] = (long) ptr;
        memcpy(ptr,s,len);
        delete[] s;
    }


    /* align on 8 bytes and make room for argc, argv, and return address */
    long userESP = (long) ptr;
    userESP = ((userESP / 8) * 8) - 16;
    long* userStack = (long*) userESP;
    userStack[0] = argc;
    userStack[1] = (long) userArgs; /* argv */

    /* clear resources */
    resources->closeAll();

    /* read ELF */
    Elf32_Ehdr hdr;

    prog->seek(0);
    prog->readFully(&hdr,sizeof(Elf32_Ehdr));

    uint32_t hoff = hdr.e_phoff;

    for (uint32_t i=0; i<hdr.e_phnum; i++) {
        Elf32_Phdr phdr;
        prog->seek(hoff);
        prog->readFully(&phdr,sizeof(Elf32_Phdr));
        hoff += hdr.e_phentsize;

        if (phdr.p_type == PT_LOAD) {
            char *p = (char*) phdr.p_vaddr;
            //uint32_t memsz = phdr.p_memsz;
            uint32_t filesz = phdr.p_filesz;

            prog->seek(phdr.p_offset);
            prog->readFully(p,filesz);
        }
    }

    switchToUser(hdr.e_entry, userESP,0);

    Debug::shutdown("What?");
    return 0;
}

bool Process::isTerminated() {
    return state == TERMINATED;
}

bool Process::isRunning() {
    return state == RUNNING;
}

bool Process::isBlocked() {
    return state == BLOCKED;
}

bool Process::isReady() {
    return state == READY;
}

void Process::makeReady() {
    disable();
    state = READY;
    if (this != idleProcess) {
        readyQueue->addTail(this);
    }
    enable();
}

/******************/
/* Static methods */
/******************/

void Process::vtrace(const char* msg, va_list ap) {
    Process *me = current;
    if (me != 0) {
        traceMutex->down();
    }

    Debug::printf("[");
    if (me) {
        if (me->name) {
            Debug::printf("%s",me->name);
        }
        Debug::printf("#%d",me->id);
    }
    Debug::printf("] ");
    Debug::vprintf(msg,ap);
    Debug::printf("\n");

    if (me != 0) {
        traceMutex->up();
    }
}

void Process::trace(const char* msg, ...) {
    va_list ap;
    va_start(ap,msg);
    vtrace(msg,ap);
    va_end(ap);
}

void Process::exit(long exitCode) {
    Process* p = current;

    if (p) {
        //trace("%s#%d %X exiting", p->name, p->id, p);
        p->inSignal = false;

        p->exitCode = exitCode;
        p->resources->closeAll();
        p->onExit();

        // signal or wake up the parent
        p->doneEvent.signal();
        if(p->parent){
            p->parent->signal(SIGCHLD);
        }

        Process::disable();
        reaperQueue->addTail(p);
        //Debug::printf("reaperQueue += %X\n",p);
        p->state = TERMINATED;
        current = nullptr;

        yield();
        Debug::panic("should never get here");
        Process::enable();
    }
}

/* switch to the next process */
void Process::dispatch(Process *prev) {
    state = RUNNING;

    uint32_t stackBottom = (uint32_t) &stack[STACK_LONGS];
    uint32_t stackBytes = stackBottom - (uint32_t) kesp;
    uint32_t stackLongs = stackBytes / 4;

    if (stackLongs >= STACK_LONGS) {
        Debug::printf("switching to %s#%d %X\n",name, id, this);
        Debug::printf("switching from %s#%d %X\n",prev ? prev->name : "?", prev ? prev->id : -1, prev);
        Debug::printf("stackBottom=%X, kesp=%X\n",stackBottom,kesp);
        Debug::panic("stack for %s %d is too big, %d bytes, iDepth:%d iCount:%d",name, id, stackBytes,iDepth,iCount);
    }

    if (this != prev) {
        addressSpace.activate();
        TSS::esp0((uint32_t) &stack[STACK_LONGS]);
        current = this;
        contextSwitch(
                prev ? &prev->kesp : 0, kesp, (disableCount == 0) ? (1<<9) : 0);
    }

    checkKilled();

    //Debug::printf("going to check signals\n");
    if( !inSignal ){ // we do not want recursive signal handling
        inSignal = true;
        Signal::checkSignals(Process::current->signalQueue);
        inSignal = false;
    }
    //    Debug::printf("checked signals\n");

}

void Process::yield(Queue<Process*> *q) {
    Process::disable();
    Process* me = current;
    if (me) {
        if (q) {
            /* a queue is specified, I'm blocking on that queue */
            if (me->iDepth != 0) {
                Debug::printf("process %s#%d %X ", me->name, me->id, me);
                Debug::panic("blocking while iDepth = %d",me->iDepth);
            }
            me->state = BLOCKED;
            q->addTail(me);

            // Debug::printf("blocking process %s#%d %X\n", me->name, me->id, me);
        } else {
            /* no queue is specified, put me on the ready queue */
            me->makeReady();
        }
    }

    Process* next;

    if (readyQueue->isEmpty()) {
        if (!idleProcess) {
            idleProcess = new IdleProcess();
            idleProcess->start();
        }
        next = idleProcess;
    } else {
        next = readyQueue->removeHead();
    }

    next->dispatch(me);
    Process::enable();
}

void Process::yield() {
    yield(nullptr);
}

signal_action_t Process::getSignalAction(signal_t s){
    switch((signal_action_t)(uint32_t)signalHandlers[s]){
        case EXIT:
        case IGNORE:
            return (signal_action_t)(uint32_t)signalHandlers[s];
        case HANDLE: // The user should never use this explicitly
        case DEFAULT:
            return IGNORE;
        default:
            return HANDLE;
    }
}

long Process::setSignalAction(signal_t sig, signal_action_t act){
    if(!Signal::validateSignal(sig) && sig < (signal_t)0x80000000){
        return ERR_NOT_POSSIBLE;
    }
    if(sig == SIGKILL) { // cannot catch, block, or ignore
        return ERR_NOT_POSSIBLE;
    }
    //    trace("sig%d = %x", sig, act);
    switch(act) {
        case DEFAULT:
            signalHandlers[sig] = (uint32_t)Signal::defaultDisposition(sig);
            break;
        default:
            signalHandlers[sig] = (uint32_t)act;
    }
    return 0;
}

/*******************/
/* The timer class */
/*******************/

class Timer {
    public:
        uint32_t target;
        Timer *next;
        SimpleQueue<Process*> waiting;
};

class Alarm : public Timer {};

void Process::sleepUntil(uint32_t second) {
    Process::disable();

    uint32_t target = second * Pit::hz;
    if (target > Pit::jiffies) {
        Timer **pp = &timers;
        Timer* p = timers;
        while (p) {
            if (p->target == target) {
                break;
            } else if (p->target > target) {
                p = nullptr;
                break;
            } else {
                pp = &p->next;
                p = p->next;
            }
        }
        if (!p) {
            p = new Timer();
            p->target = target;
            p->next = *pp;
            *pp = p;
        }
        Process::yield(&p->waiting);
    }

    Process::enable();
}

void Process::sleepFor(uint32_t seconds) {
    sleepUntil(Pit::seconds() + seconds);
}

long Process::alarm(uint32_t second) {
    Process::disable();

    //trace("disposition of sigalrm = %d", Process::current->getSignalAction(SIGALRM));

    uint32_t target = second * Pit::hz + Pit::jiffies;
    if (target > Pit::jiffies) {
        Alarm **pp = &alarms;
        Alarm* p = alarms;
        while (p) {
            if (p->target == target) {
                break;
            } else if (p->target > target) {
                p = nullptr;
                break;
            } else {
                pp = (Alarm**) &p->next;
                p = (Alarm*) p->next;
            }
        }
        if (!p) {
            p = new Alarm();
            p->target = target;
            p->next = *pp;
            *pp = p;
        }
        p->waiting.addTail(Process::current);
        Process::enable();
    } else {
        Process::enable();
        Process::current->signal(SIGALRM);
    }

    return 0;
}

/* called for every timer tick */
void Process::tick() {
    /* interrupts are already disabled but might as well */
    Process::disable();

    //Debug::printf("%d sec\n", Pit::seconds());

    if (current == idleProcess) {
        idleJiffies ++;
    }

    Timer* first = timers;
    if (first) {
        if (Pit::jiffies == first->target) {
            timers = first->next;
            while (!first->waiting.isEmpty()) {
                Process* p = first->waiting.removeHead();
                p->makeReady();
            }
            delete first;
        }
    }

    Alarm* firstAl = alarms;
    if (firstAl) {
        if (Pit::jiffies == firstAl->target) {
            //          Debug::printf("alarm %d at time %d\n", firstAl->target / Pit::hz, Pit::seconds());
            alarms = (Alarm*)firstAl->next;
            while (!firstAl->waiting.isEmpty()) {
                Process* p = firstAl->waiting.removeHead();
                p->signal(SIGALRM);
            }
            delete firstAl;
        }
    }

    Process::enable();
}

void Process::disable() {
    Pic::off();
    Process* me = current;
    if (me) {
        me->disableCount ++;
    }
}

void Process::enable() {
    Process* me = current;
    /* It is meaningless to enable interrupts without
       a current process */
    if (me) {
        uint32_t c = me->disableCount;
        if (c == 0) {
            Debug::panic("disable = %d",c);
        } else if (c == 1) {
            me->disableCount = 0;
            Pic::on();
        } else {
            me->disableCount --;
        }
    }
}

void Process::startIrq() {
    Process* me = Process::current;
    if (current == 0) {
        Debug::panic("startIrq with no process");
        return;
    }
    me->iCount ++;
    me->iDepth ++;
    if (me->disableCount != 0) {
        Debug::panic("disableCount = %d",me->disableCount);
    }
    me->disableCount = 1;
}

void Process::endIrq() {
    Process* me = Process::current;
    if (me == 0) {
        return;
    }
    me->iDepth --;
    if (me->disableCount != 1) {
        Debug::panic("disableCount = %d",me->disableCount);
    }
    me->disableCount = 0;
}
