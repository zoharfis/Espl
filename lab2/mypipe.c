#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int piperoom[2];
    char buff[100];
    int pid, len;
    // open pipe
    if (pipe(piperoom) == -1) {
        perror("Error open pipe");
        exit(1);
    }
    // creates a child process
    pid = fork();
    if (pid == 0) { // child process
        close(piperoom[0]);
        printf("Enter message:\n");
        fgets(buff, sizeof(buff), stdin);
        len = strlen(buff) - 1;
        write(piperoom[1], buff, len);
        close(piperoom[1]);
    }
    else { // parent process
        close(piperoom[1]);
        read(piperoom[0],buff, sizeof(buff));
        printf("Parent incoming message: %s\n", buff);
        close(piperoom[0]);
        exit(0);
    }
    return 0;
}