#include "syscall.h"
#include "machine.h"
#include "idt.h"
#include "process.h"
#include "child.h"
#include "fs.h"
#include "err.h"
#include "u8250.h"
#include "libk.h"
#include "pic.h"

void Syscall::init(void) {
    IDT::addTrapHandler(100,(uint32_t)syscallTrap,3);
}

extern "C" long syscallHandler(uint32_t* context, long num, long a0, long a1) {

    switch (num) {
        case 0: /* exit */
            //Process::trace("exiting");
            Process::exit(a0);
            return -1;
        case 1: /* putchar */
            //Debug::printf("disableCount=%d, iDepth=%d\n", Process::current->disableCount, Process::current->iDepth);
            Debug::printf("%c",a0);
            return 0;
        case 2: /* fork */
            {
                uint32_t userPC = context[8];
                uint32_t userESP = context[11];
                Child *child = new Child(Process::current);
                child->pc = userPC;
                child->esp = userESP;
                child->eax = 0;
                long id = Process::current->resources->open(child);
                child->start();

                return id;
            }
        case 3: /* semaphore */
            {
                Semaphore *s = new Semaphore(a0);
                return Process::current->resources->open(s);
            }
        case 4: /* down */
            {
                Semaphore* s = (Semaphore*) Process::current->resources->get(
                        a0,ResourceType::SEMAPHORE);
                if (s == nullptr) return ERR_INVALID_ID;
                s->down();
                return 0;
            }
        case 5 : /* up */
            {
                Semaphore* s = (Semaphore*) Process::current->resources->get(a0,
                        ResourceType::SEMAPHORE);
                if (s == nullptr) return ERR_INVALID_ID;
                s->up();
                return 0;
            }
        case 6 : /* join */
            {
                Process *proc = (Process*) Process::current->resources->get(a0,
                        ResourceType::PROCESS);
                if (proc == nullptr) return ERR_INVALID_ID;
                proc->doneEvent.wait();
                long code = proc->exitCode;
                Process::current->resources->close(a0);
                return code;
            }
        case 7 : /* shutdown */
            {
                Debug::shutdown("");
                return 0;
            }
        case 8 : /* open */
            {
                File* f = FileSystem::rootfs->rootdir->lookupFile((char*) a0);
                if (f == nullptr) return ERR_NOT_FOUND;
                else return Process::current->resources->open(f);
            }
        case 9 : /* getlen */
            {
                File* f = (File*) Process::current->resources->get(a0,ResourceType::FILE);
                if (f == nullptr) {
                    return ERR_INVALID_ID;
                }
                return f->getLength();
            }
        case 10: /* read */
            {
                long *args = (long*) a0;
                File* f = (File*) Process::current->resources->get(args[0],ResourceType::FILE);
                if (f == nullptr) {
                    return ERR_INVALID_ID;
                }
                void* buf = (void*) args[1];
                long len = (long) args[2];
                return f->read(buf,len);
            }
        case 11 : /* seek */
            {
                File* f = (File*) Process::current->resources->get(a0,ResourceType::FILE);
                if (f == nullptr) {
                    return ERR_INVALID_ID;
                }
                f->seek(a1);
                return 0;
            }
        case 12 : /* close */
            {
                return Process::current->resources->close(a0);
            }
        case 13: /* execv */
            {
                /* find the security exposures in this code */
                char* name = (char*) a0;
                char** userArgs = (char**) a1;

                SimpleQueue<const char*> args;

                int i = 0;

                while(true) {
                    char* s = K::strdup(userArgs[i]);
                    if (s == 0) break;
                    args.addTail(s);
                    i++;
                }

                long rc = Process::current->execv(name,&args,i);

                /* execv failed, cleanup */
                while (!args.isEmpty()) {
                    const char* s = args.removeHead();
                    delete[] s;
                }
                return rc;
            }
        case 14: /* getchar */
            {
                return U8250::it->get();
            }
        case 15: /* kill */
            {
                //Process::trace("made it to syscallHandler");
                Process *proc = (Process*) Process::current->resources->get(a0,
                        ResourceType::PROCESS);
                if (proc == nullptr) return ERR_INVALID_ID;
                //Process::trace("sending signal %d to %s %d", a1, proc->name, proc->id);
                proc->signal((signal_t)a1);
                //Process::trace("done");
                return 0;
            }
        case 16: /* signal */
            {
                //Debug::printf("iDepth = %d\n", Process::current->iDepth);
                //Debug::printf("signal %d is %X\n", a0, a1);
                return Process::current->setSignalAction((signal_t)a0, (signal_action_t)a1);
            }
        case 17: /* alarm */
            {
                return Process::current->alarm((uint32_t)a0);
            }
        case 18: /* mmap */
            {
                if((uint32_t)a0 < 0x400000 || (uint32_t)a0 >= 0x80000000){
                    return ERR_NOT_POSSIBLE;
                }
                return Process::current->addressSpace.mmap((uint32_t)a0 >> 12 << 12);
            }
        case 0xff: /* sys_sigret */
            {
                // interrupts are disabled
                //Process::trace("sys_sigret");

                //Debug::printf("uesp=%X\n", Process::current->context->frame->registers.esp);
                //Debug::printf("upc=%X\n", Process::current->context->frame->registers.eip);

                // NOTE: DO NOT USE Process::current->context->registers, because they are stale!!

                Process::current->inSignal = false;
                sys_sigret((uint32_t)&(Process::current->context->frame->registers));

                Debug::panic("What?");
                return -1;
            }
        default:
            Process::trace("syscall(%d,%d,%d)",num,a0,a1);
            return -1;
    }
}
