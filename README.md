## PanicOS ##
=============

This project implements Linux-style signals. It is not Linux-compliant, though.

Signals
-------
So far this project has the following signals:
<table>
    <tr>
        <th>Signal</th>
        <th>Value</th>
        <th>Default disposition</th>
    </tr>
    <tr>
        <td><code>SIGINT</code></td>
        <td>2</td>
        <td>Term</td>
    </tr>
    <tr>
        <td><code>SIGKILL</code></td>
        <td>9</td>
        <td>Term</td>
    </tr>
    <tr>
        <td><code>SIGALRM</code></td>
        <td>14</td>
        <td>Term</td>
    </tr>
    <tr>
        <td><code>SIGCHLD</code></td>
        <td>17</td>
        <td>Ign</td>
    </tr>
</table>

User API
--------
The "libc" signals API includes a few fundamental definitions.

```
long kill(long pd, long sig);
long signal(long sig, void *sighandler);
long alarm(long seconds);
long sigreturn();
```

`kill` sends the given signal to the process identified by the process descriptor `pd`. Because there is no global process table in this kernel, we cannot send a signal to a PID. The valid values for the signal are 2 (`SIGINT`), 9 (`SIGKILL`), 14 (`SIGALRM`), and 17 (`SIGCHLD`). The signal values are the same as in Linux, and the signals behave similarly to those of Linux. "libc" defines macros for the signals, so the user can use `SIGKILL`, rather than 9, for example.

`signal` sets the disposition of a signal. Its first parameter is the signal, and its second signal is either (a) a pointer to a signal handler, (b) `SIG_IGN` which sets the disposition to "Ignore", or (c) `SIG_DFL` which sets the disposition to the default. `SIGKILL` always kills the process, and it cannot be caught or ignored.

`alarm` arranges for `SIGALRM` to be sent to this process after the given number of seconds.

`sigreturn` returns from a signal handler. It should not be called explicitly.

Signal handlers can accept one parameter if they chose to: a pointer to a struct of type `regs`. The following `typedef` is provided in "libc".
```
typedef struct {
    unsigned long cr2;
	unsigned long ds;
	unsigned long ebp;
	unsigned long edi;
	unsigned long esi;
	unsigned long edx;
	unsigned long ecx;
	unsigned long ebx;
	unsigned long eax;
	unsigned long eip;
	unsigned long cs;
	unsigned long flags;
	unsigned long esp;
	unsigned long ss;
} regs;

```
This struct is mutable, and changes to it are reflected in the process's execution when it returns from the signal handler.

Implementation
==============

Processes
----------------------
Processes now have a few extra instance members:
+ a queue of unhandled signals.
+ a mutex to protect the queue.
+ a boolean value denoting whether the signal is in a signal handler currently. This value is used to avoid deadlocks and race conditions without blocking (as a mutex would). I may decide to implement a `tryAndLock()` method for mutexes later.
+ a list of signal handlers and dispositions.
+ a context struct that saves the user-mode context of this process whenever we switch to kernel-mode.

They also have a few extra methods:
+ `void signal(signal_t sig)` adds the signal `sig` to the signal queue. The signal value must be validated beforehand.
+ `virtual signal_action_t getSignalAction(signal_t)` to get the disposition of this signal for this process.
+ `virtual long setSignalAction(signal_t, signal_action_t)` to set the disposition of this signal for this process.

The internal signal API
-----------------------
The bulk of the kernel's signal implementation is housed in `signal.h` and `signal.cc`.

`signal.h` defines a number of important structs and enums:
+ `regs` contains the values of the processor registers.
+ `signal_t` defines the different signals.
+ `signal_action_t` defines the different dispositions.
+ `sigframe` represents a signal handler's stack frame.
+ `sigcontext` represents the user-mode context of a process. It contains a pointer to the signal handler frame, if there is one, which is used by `sys_sigret`.

The `Signal` class represents a signal and encapsulates the logic of switching to a signal handler. It has the following declaration:
```
class Signal{
private:
    signal_t sig;

    void setupFrame();
    sigframe *getSignalFrame(jumpercode*);
    jumpercode *putJumperCode();

public:
    Signal(signal_t sig) : sig(sig) {}

    static void checkSignals(SimpleQueue<Signal*> *signals);
    static signal_action_t defaultDisposition(signal_t);
    static void initHandlers(uint32_t (&handlers)[SIGNUM]);
    static bool validateSignal(signal_t s);
    void doSignal();
};
```

+ `checkSignals` dequeues a single signal from the current process and handles it.
+ `defaultDisposition` returns the default disposition of the given signal
+ `initHandlers` initializes the given list of signal handlers and dispositions.
+ `validateSignal` returns true if the given signal is valid (false otherwise).

These methods are mostly used for bookkeeping by processes.

The `doSignal` method checks a signal's disposition (by calling `Process::getSignalAction`) and acts accordingly. If the dispositon is `HANDLE`, we prepare a stack frame and jump to the signal handler (as described below).

The remaining protion of the internal signal API is defined in `machine.S` and `syscall.cc`. `machine.S` contains the `sys_sigret` function which returns to the user execution (where the signal interrupted). `syscall.cc` contains the definitions of the system calls listed above. Both are pretty straight-forward, so I will not go into details.

There and back again
--------------------
When the kernel needs to call a signal handler, several things happen:
1. a stack frame is created for the handler in user space.
2. jumper code is put on the user stack to return execution to kernel space after the signal handler returns.
3. the user context (a `regs` struct) is copied to the user stack, and a pointer to it is put in the stack frame as a parameter to the handler.
4. the process's `sigcontext` is updated to point to the user's copy of the registers.

After everything is in place, `switchToUser` is called to go to user-mode. Voila! We are running in the signal handler.

When the signal handler returns, it goes to the jumper code we set up. This calls the `sigreturn` system call. `sigreturn` simply calls `sys_sigret`, passing it a pointer to the `regs` struct on the user stack. `sys_sigret` restore this context and executes an `iret` instruction bringing us back to the normal execution of the process.

Challenges
----------
+ Understanding this mechanism was a long journey. I had originally implemented a much more convoluted system to go to and come back from signal handlers.
+ Concurrency was a pain. Solving a deadlock often introduced a race condition, and vice versa. Also, deciding when to use a mutex (as opposed to disabling interrupts) was often tricky.

Make
----

To build

```
make
```

To run with QEMU

```
make run
```

To debug with QEMU and GDB

```
make debug
```
then in another window
```
gdb kernel/kernel
(gdb) target remote localhost:1234
(gdb) b <breakpoint>
(gdb) c
```
