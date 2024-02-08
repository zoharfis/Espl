#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

char *argvls[] = {"ls", "-l", NULL};
char *argvtail[] = {"tail", "-n", "2", NULL};

void execute(const char* command, char* argv[]) {
    execvp(command, argv);
    perror("fail running the program");
    exit(1);
}

int main(int argc, char **argv) {
    int piperoom[2];
    int pid1, pid2;

    // open pipe
    if (pipe(piperoom) == -1) {
        perror("Error open pipe");
        exit(1);
    }
    // creates child1 process
    fprintf(stderr, "(parent_process>forking…)\n");
    pid1 = fork();
    // child1 process
    if (pid1 == 0) { 
        close(piperoom[0]);
        // redirecting stdout to pipe
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        close(1); // close the standart output stream
        dup(piperoom[1]); // duplicate the write-end of the pipe
        close(piperoom[1]);
        // execute
        fprintf(stderr, "(child1>going to execute cmd: …)\n");
        execute("ls", argvls);
    }
    // parent process
    fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);
    // close the write-end of the pipe
    fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
    close(piperoom[1]);
    // creates child2 process
    fprintf(stderr, "(parent_process>forking…)\n");
    pid2 = fork();
    // child2 process
    if (pid2 == 0) {
        close(piperoom[1]);
        // redirecting stdin to pipe
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
        close(0); // close the standart input stream
        dup(piperoom[0]); // duplicate the read-end of the pipe
        close(piperoom[0]);
        // execute
        fprintf(stderr, "(child2>going to execute cmd: …)\n");
        execute("tail", argvtail);
    }
    // parent process
    fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);
    // close the read-end of the pipe
    fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
    close(piperoom[0]);
    // wait for the child processes to terminate
    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    fprintf(stderr, "(parent_process>exiting…)\n");
    return 0;
}