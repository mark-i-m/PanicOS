#ifndef _USER_SIGNAL_H_
#define _USER_SIGNAL_H_

// Signals
#define SIGINT (2)
#define SIGALRM (14)
//#define SIGSEGV (11)
#define SIGCHLD (17)
#define SIGKILL (9)

// Dispositions
#define SIG_IGN (0)
#define SIG_DFL (1)

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

#endif
