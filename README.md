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
+ a context struct that saves the user-mode context of this process.

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

The `Signal` class represents a signal and encapsulates the logic of switching to a signal handler. It contains the following methods:


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
