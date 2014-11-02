#include "libc.h"

char** split(char *str) {
    long nWords = 0;
    long nArgs = 0;
    char **args = 0;
    int i = 0;

    int space = 1;

    while (1) {
        char c = str[i];
        if (c == 0) break;

        if (c == ' ') {
            str[i] = 0;
            space = 1;
        } else {
            if (space) {
                if (nArgs <= nWords) {
                    nArgs = nWords + 10;
                    args = (char**) realloc(args, (nArgs + 1) * 4);
                    if (args == 0) return 0;
                }
                args[nWords] = &str[i];
                nWords ++;
            }
            space = 0;
        }
        i++;
    }

    if (args) {
        args[nWords] = 0;
    }

    return args;
        
}

void notFound(char* cmd) {
    puts(cmd);
    puts(": command not found\n");
}

int main() {
    while (1) {
        puts("shell> ");
        char* in = gets();
        char **args = 0;

        if (in == 0) goto done;

        args = split(in);
        if (args == 0) goto done;
        
        char *cmd = args[0];
        if (cmd == 0) goto done;

        int fd = open(cmd);
        if (fd < 0) {
            notFound(cmd);
            goto done;
        }

        int magic = 0;
        readFully(fd,&magic,4);
     
        if (magic == 0x464c457f) {
            /* executable file */
            close(fd);

            long id = fork();
            if (id == 0) {
                /* child */
                long rc = execv(cmd,args);
                notFound(cmd);
                exit(rc);
            } else {
                join(id);
            }
        } else {
            /* write it */
            seek(fd,0);
            char buf[100];
            while (1) {
                int n = read(fd,buf,100);
                if (n == 0) break;
                if (n < 0) {
                    notFound(cmd);
                    break;
                }
                for (int j=0; j<n; j++) {
                    putchar(buf[j]);
                }
            }
            close(fd);
        }
done:
        if (in) free(in);
        if (args) free(args);
    }
    return 0;
}
