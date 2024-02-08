#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "LineParser.h"
#include <linux/limits.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0

#define HISTLEN 20

#define FREE(X) if(X) free((void*)X)

typedef struct process{
        cmdLine* cmd;                         /* the parsed command line*/
        pid_t pid; 		                  /* the process id that is running the command*/
        int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
        struct process *next;	                  /* next process in chain */
    } process;

typedef struct history{
        char *historyArr[HISTLEN]; /* the commands queue*/
        int newest;                /* the last command*/
        int oldest;                /* the oldest command that saved in queue*/
        int size;                  /* current size of historyArr*/
    } history;

/* add new process to process_list. */
void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process *newProc = (process*)malloc(sizeof(process));
    newProc->cmd = cmd;
    newProc->pid = pid;
    newProc->status = RUNNING;
    newProc->next = (*process_list);
    (*process_list) = newProc;
}

/* print one process */
void printProcess(process *proc) {
    char status[12];
    int i;
    switch (proc->status)
    {
    case TERMINATED:
        strcpy(status, "TERMINATED");
        break;
    case RUNNING:
        strcpy(status, "RUNNING");
        break;
    case SUSPENDED:
        strcpy(status, "SUSPENDED");
        break;    
    default:
        break;
    }
    fprintf(stdout, "%d %s ", proc->pid, status);
    for (i = 0; i < proc->cmd->argCount; i++) {
        if (i == proc->cmd->argCount - 1)
            printf("%s\n", proc->cmd->arguments[i]); 
        else 
            printf("%s ", proc->cmd->arguments[i]);
    }
}

/* free one cmdLine only. */
void freeCmdLine(cmdLine* cmd) {
    int i;
    if (cmd != NULL) {
        FREE(cmd->inputRedirect);
        FREE(cmd->outputRedirect);
        for (i = 0; i < cmd->argCount; i++) {
            FREE(cmd->arguments[i]);
        }
        free(cmd);
    }
}

/* free one process only. */
void freeOneProcess(process* proc) {
    if (proc != NULL) {
        freeCmdLine(proc->cmd);
        free(proc);
    }
}

/* free all memory allocated for the process list. */
void freeProcessList(process* process_list) {
    if (process_list != NULL) {
        freeProcessList(process_list->next);
        freeOneProcess(process_list);
    }
}

/* delete all terminated processes from process_list. */
void deleteDeadProc(process** process_list) {
    process *proc = (*process_list), *befProc = NULL, *delProc = NULL;
    while (proc != NULL) {
        if (proc->status == TERMINATED) { // delete
            delProc = proc;
            if (befProc == NULL) { // first in list
                (*process_list) = proc->next;
            }
            else {
                befProc->next = proc->next;
            }
            proc = proc->next;
            freeOneProcess(delProc);
        }
        else {
            befProc = proc;
            proc = proc->next;
        }
    }
}

/* find the process with the given id in the process_list and change its status to the received status. */
void updateProcessStatus(process* process_list, int pid, int status) {
    while (process_list != NULL) {
        if (process_list->pid == pid) {
            process_list->status = status;
            return;
        }
        process_list = process_list->next;
    }
}

/* update status for done processes in process_list. */
void updateProcessList(process **process_list) {
    int id, stat;
    process *proc = (*process_list);
    while (proc != NULL) {
        id = waitpid(proc->pid, &stat, WNOHANG | WUNTRACED | WCONTINUED);
        if (id == -1) {
            proc->status = TERMINATED; 
        }
        else if (id > 0) {
            if (WIFCONTINUED(stat))
                proc->status = RUNNING;
            else if (WIFSTOPPED(stat)) 
                proc->status = SUSPENDED;
            else if (WIFEXITED(stat) || WIFSIGNALED(stat))
                proc->status = TERMINATED;
        }
        proc = proc->next;
    }
}

/* update and print the processes. */
void printProcessList(process** process_list) {
    process *proc;
    int index = 0;
    if (process_list == NULL)
        return;
    updateProcessList(process_list);
    proc = (*process_list);
    // print process list
    while (proc != NULL) {
        fprintf(stdout, "%d ", index);
        printProcess(proc);
        proc = proc->next;
        index++;
    }
    deleteDeadProc(process_list);
}

/* build history mechine and return it. */
history *makeHistory() {
    history *his = (history*)malloc(sizeof(history));
    his->newest = -1;
    his->oldest = -1;
    his->size = 0;
    return his;
}

/* add newCmd to the queue in history. */
void enqueueHistory(history *his, char *newCmd) {
    if (his->size == HISTLEN) { // full queue
        free(his->historyArr[his->oldest]);
        his->historyArr[his->oldest] = newCmd;
        his->newest = his->oldest;
        his->oldest = (his->newest + 1) % HISTLEN;
    }
    else {
        if (his->size == 0) { // first command
            his->historyArr[0] = newCmd;
            his->newest = 0;
            his->oldest = 0;
            his->size++;
        }
        else {
            his->newest = (his->newest + 1) % HISTLEN;
            his->historyArr[his->newest] = newCmd;
            his->size++;
        }
    }
}

/* free all memory allocated for his. */
void freeHistory(history *his) {
    int i;
    for (i = 0; i < his->size; i++) {
        free(his->historyArr[i]);
    }
    free(his);
}

/* print the history. */
void printHistory(history *his) {
    int i = his->oldest;
    int count = 0;
    while (count < his->size) {
        printf("array[%d]: %s", i + 1, his->historyArr[i]);
        i = (i + 1) % HISTLEN;
        count++;
    }
}

/* receives a parsed line and invokes the program specified in the cmdLine using the proper system call. */
void execute(cmdLine *pCmdLine) {
    execvp(pCmdLine->arguments[0], pCmdLine->arguments);
    perror("fail running the program");
    exit(1);
}

/* check debugMode and print debugging messages accordingly. */
void debugging(cmdLine *cmd, int pid) {
    int i, isDebug = 0;
    // check debugMode
    for (i = 1; i < cmd->argCount; i++) {
        if (strcmp(cmd->arguments[i], "-d") == 0) {
           isDebug = 1;
           break;
        }
    }
    // print debug information
    if (isDebug) {
        fprintf(stderr, "PID: %d\n", pid);
        fprintf(stderr, "Executing command: %s\n", cmd->arguments[0]);
    }
}

/* wait for the child process pid to terminate if its blocking process. */
void waiting(char blocking, int pid) {
    if (blocking)
        waitpid(pid, NULL, 0);
}

/* handle redirecting and execute the command. */
void executeCmd(cmdLine *cmd) {
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

/* check if cmd is signal command and do it. return 1 for signal command or 0 else. */
int signalsCmd(cmdLine *cmd, process *procList) {
    int pid;
    int sig;
    int status;
    if (strcmp(cmd->arguments[0], "suspend") != 0 && 
        strcmp(cmd->arguments[0], "wake") != 0 && 
        strcmp(cmd->arguments[0], "kill") != 0)
        return 0;
    if (cmd->argCount != 2) { // illegal command
        fprintf(stderr, "Error illegal arguments for signal command.\n");
        return 1;
    }
    sscanf(cmd->arguments[1], "%d", &pid);
    if (strcmp(cmd->arguments[0], "suspend") == 0) {
        sig = SIGTSTP;
        status = SUSPENDED;
    }
    else if (strcmp(cmd->arguments[0], "wake") == 0) {
        sig = SIGCONT;
        status = RUNNING;
    }
    else { // kill
        sig = SIGINT;
        status = TERMINATED;
    }
    if (kill(pid, sig) != 0) { 
        perror("Error failed send signal"); // error
    }
    else { // succeed
        printf("Signal sent.\n");
        updateProcessStatus(procList, pid, status);
    }
    return 1;
}

/* check if cmd is cd command and do it. return 1 for cd command or 0 else. */
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
        return 1;
    }
    return 0;
}

/* check if cmd is procs command and do it. return 1 for procs command or 0 else. */
int procsCmd(cmdLine *cmd, process **procListP) {
    if (strcmp(cmd->arguments[0], "procs") == 0) {
        if (cmd->argCount != 1) { // illegal command
            fprintf(stderr, "Error illegal arguments for procs.\n");
        }
        else {
            printProcessList(procListP);
        }
        return 1;
    }
    return 0;
}

/* check if cmd is history command and do it. return 1 for history command or 0 else. */
int historyCmd(cmdLine *cmd, history *his) {
    if (strcmp(cmd->arguments[0], "history") == 0) {
        if (cmd->argCount != 1) { // illegal command
            fprintf(stderr, "Error illegal arguments for history.\n");
        }
        else {
            printHistory(his);
        }
        return 1;
    }
    return 0;
}

/* check if cmd is retrieve command and return index in historyArr, -1 if not retrieve or -2 for illegal retrieve. */
int retrieveCmd(cmdLine *cmd, history *his) {
    int num, sscan;
    if (strcmp(cmd->arguments[0], "!!") == 0) {
         if (cmd->argCount != 1) { // illegal command
            fprintf(stderr, "Error illegal arguments for !!.\n");
            return -2;
        }
        if (his->size == 0) {
            fprintf(stderr, "Error no history for command for !!.\n");
            return -2;
        }
        return his->newest;
    }
    if (cmd->arguments[0][0] == '!') {
        if (cmd->argCount != 1) { // illegal command too many arguments
            fprintf(stderr, "Error illegal arguments for !.\n");
            return -2;
        }
        sscan = sscanf(cmd->arguments[0] + 1, "%d", &num);
        if (sscan == EOF || sscan == -1 || sscan == 0) { // illegal number
            fprintf(stderr, "Error illegal number for !.\n");
            return -2;
        }
        num = num - 1;
        if (his->newest != -1 &&
            ((his->newest <= his->oldest && 
            ((his->newest >= num && num >= 0) || (his->oldest <= num && num < HISTLEN)))
            || (his->newest >= his->oldest && his->oldest <= num && num <= his->newest))) { // legal number
            return num;
        }
        else { // invalid number
            fprintf(stderr, "Error invalid number - no history for number %d.\n", num + 1);
            return -2;
        }
    }
    return -1;
}

/* handle command and perfom action accordingly. */
void handleCmd(cmdLine *cmd, process **procListP, history *his) {
    int pid;
    if (!(signalsCmd(cmd, (*procListP))) && !(cdCmd(cmd)) && !(procsCmd(cmd, procListP)) && !(historyCmd(cmd, his))) { // check shell commands
        if (!(pid = fork())) { // child process
            executeCmd(cmd);
        }   
        // parent process
        debugging(cmd, pid);
        addProcess(procListP, cmd, pid);
        waiting(cmd->blocking, pid);
    }
    else { // no new process
        freeCmdLines(cmd);
    }
}

/* handle commands with pipe. */
void handlePipe(cmdLine *cmd, process **procListP, history *his) {
    int piperoom[2];
    int pid1, pid2;

    if (cmd->outputRedirect != NULL || cmd->next->inputRedirect != NULL) {
        fprintf(stderr, "Error Illegal redirecting with pipe.\n");
        return;
    }
    // open pipe
    if (pipe(piperoom) == -1) {
        perror("Error open pipe");
        freeCmdLines(cmd);
        freeProcessList((*procListP));
        freeHistory(his);
        kill(0, SIGTERM);
        exit(1);
    }
    pid1 = fork();
    // child1 process
    if (pid1 == 0) { 
        close(piperoom[0]);
        // redirecting stdout to pipe
        close(1);
        dup(piperoom[1]);
        close(piperoom[1]);
        // execute
        executeCmd(cmd);
    }
    // parent process
    debugging(cmd, pid1);
    addProcess(procListP, cmd, pid1);
    close(piperoom[1]); // close the write-end of the pipe
    pid2 = fork();
    // child2 process
    if (pid2 == 0) {
        close(piperoom[1]);
        // redirecting stdin to pipe
        close(0);
        dup(piperoom[0]);
        close(piperoom[0]);
        // execute
        executeCmd(cmd->next);
    }
    // parent process
    debugging(cmd->next, pid2);
    addProcess(procListP, cmd->next, pid2);
    close(piperoom[0]); // close the read-end of the pipe
    waiting(cmd->blocking, pid1);
    waiting(cmd->next->blocking, pid2);
}

int main(int argc, char **argv) {
    char cwd[PATH_MAX];
    char buf[2048];
    cmdLine *cmd = NULL;
    process *processList = NULL;
    process **processListP = &processList;
    char *strCmd = NULL;
    history *his = makeHistory();
    int num;

    while (1) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) 
            fprintf(stderr, "Error getting path.\n");
        printf("Current working directory: %s\n", cwd);
        if (fgets(buf, sizeof(buf), stdin) == NULL) { // read from stdin
            freeProcessList(processList);
            freeHistory(his);
            kill(0, SIGTERM);
            exit(1);
        }
        // quit
        if (strcmp(buf, "quit\n") == 0) {
            freeProcessList(processList);
            freeHistory(his);
            kill(0, SIGTERM);
            exit(0);
        }
        
        cmd = parseCmdLines(buf);
        if (cmd == NULL) {
            continue;
        }
        num = retrieveCmd(cmd, his);
        if (num >= 0) { // retrieve command
            freeCmdLines(cmd);
            strcpy(buf, his->historyArr[num]);
            cmd = parseCmdLines(buf);
        }
        else if (num == -2) {
            freeCmdLines(cmd);
            continue;
        }     
        // enqueue to history 
        strCmd = malloc(strlen(buf) + 1);
        strcpy(strCmd, buf);
        enqueueHistory(his, strCmd);
        
        if (cmd->next != NULL) { // pipe
            handlePipe(cmd, processListP, his);
        }
        else { // one command
            handleCmd(cmd, processListP, his);
        }
        cmd = NULL;
    }
    freeProcessList(processList);
    freeHistory(his);
    kill(0, SIGTERM);
    return 0;
}