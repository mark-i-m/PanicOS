#ifndef _SYS_H_
#define _SYS_H_

extern long exit(long status);
extern long execv(char* prog, char** args);
extern long open(char *name);
extern long getlen(long);
extern long close(long);
extern long read(long f, void* buf, long len);
extern long seek(long f, long pos);
extern long putchar(int c);
extern long getchar();
extern long semaphore(long n);
extern long up(long sem);
extern long down(long sem);
extern long fork();
extern long join(long proc);
extern long shutdown();
extern long kill(long pd, long sig);
extern long signal(long sig, void *sighandler);
extern long alarm(long seconds);
extern long sigreturn();

#endif
