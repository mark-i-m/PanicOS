#ifndef _USER_SIGNAL_H_
#define _USER_SIGNAL_H_

#define SIGTEST (0)
#define SIGALRM (1)
#define SIGSEGV (2)

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
