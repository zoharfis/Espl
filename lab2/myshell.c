#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "LineParser.h"
#include <linux/limits.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

/* receives a parsed line and invokes the program specified in the cmdLine using the proper system call. */
void execute(cmdLine *pCmdLine) {
    execvp(pCmdLine->arguments[0], pCmdLine->arguments);
    perror("fail running the program");
    freeCmdLines(pCmdLine);
    exit(1);
}

/* execute the command */
void executeCmd(cmdLine *cmd) {
    int pid, i, isDebug = 0;

    if (!(pid = fork())) { // child process
        // redirecting input 
        if (cmd->inputRedirect != NULL){
            close(0); // input stream file descriptor
            fopen(cmd->inputRedirect, "r");
        }
        // redirecting output
        if (cmd->outputRedirect != NULL){
            close(1); // output stream file descriptor
            fopen(cmd->outputRedirect, "w");
        }
        execute(cmd); // exit if fail
    }
    // parent process
    // check debugMode
    for (i = 1; i < cmd->argCount; i++) {
        if (strcmp(cmd->arguments[i], "-d") == 0)
           isDebug = 1;
    }
    // debug information
    if (isDebug) {
        fprintf(stderr, "PID: %d\n", pid);
        fprintf(stderr, "Executing command: %s\n", cmd->arguments[0]);
    }

    if (cmd->blocking) {
        waitpid(pid, NULL, 0);
    }
}

/* check if cmd is signal command and do it. return 0 for signal command or 1 else. */
int signalsCmd(cmdLine *cmd) {
    int pid;
    int sig;
    if (strcmp(cmd->arguments[0], "suspend") != 0 && 
        strcmp(cmd->arguments[0], "wake") != 0 && 
        strcmp(cmd->arguments[0], "kill") != 0)
        return 1;
    if (cmd->argCount != 2) { // illegal command
        fprintf(stderr, "Error illegal arguments for signal command.\n");
        return 0;
    }
    sscanf(cmd->arguments[1], "%d", &pid);
    if (strcmp(cmd->arguments[0], "suspend") == 0) sig = SIGTSTP;
    else if (strcmp(cmd->arguments[0], "wake") == 0) sig = SIGCONT;
    else sig = SIGINT; // kill
    if (kill(pid, sig) != 0) { 
        perror("Error failed send signal"); // error
    }
    else { // succeed
        printf("Signal sent.\n");
    }
    return 0;
}

/* check if cmd is cd command and do it. return 0 for cd command or 1 else. */
int cdCmd(cmdLine *cmd) {
    if (strcmp(cmd->arguments[0], "cd") == 0) {
        if (cmd->argCount != 2) { // illegal command
            fprintf(stderr, "Error illegal arguments for cd.\n");
        }
        else {
            if (chdir(cmd->arguments[1]) != 0) {
                perror("cd operation fails");
            }
        }
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    char cwd[PATH_MAX];
    char buf[2048];
    cmdLine *cmd = NULL;

    while (1) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) 
            fprintf(stderr, "Error getting path.\n");
        printf("Current working directory: %s\n", cwd);
        if (fgets(buf, sizeof(buf), stdin) == NULL) { // read from stdin
            exit(1);
        }
        // quit
        if (strcmp(buf, "quit\n") == 0) {
            freeCmdLines(cmd);
            exit(0);
        }
        
        cmd = parseCmdLines(buf);
        if (cmd == NULL) {
            continue;
        }
        // check signals ans cd commands
        if (signalsCmd(cmd) && cdCmd(cmd)) { // true if not signal and not cd
            // execute command
            executeCmd(cmd);
        }
        freeCmdLines(cmd);
        cmd = NULL;
    }
    return 0;
}