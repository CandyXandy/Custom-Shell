/* execute.c - code used by small shell to execute commands */

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <signal.h>
#include    <sys/wait.h>
#include    <string.h>
#include    "smsh.h"
#include    <fcntl.h>
#include    <sys/stat.h>

int execute(char *argv[], char *inFile, char *outFile)
/*
 * purpose: run a program passing it arguments
 * THIS IS FOR NO PIPES IN COMMANDLIST
 * returns: status returned via wait, or -1 on error
 *  errors: -1 on fork() or wait() errors
 */
{
    int pid;
    int child_info = -1;
    int inFD, outFD;

    if (argv[0] == NULL)        /* nothing succeeds	*/
        return 0;

    if ((pid = fork()) == -1)
        perror("fork");
    else if (pid == 0) {
        if (inFile != NULL || outFile != NULL) { // if we have a file to redirect
            if (inFile != NULL) {
                inFD = open(inFile, O_RDWR | O_CREAT);
                fchmod(inFD, 0777);
            }
            if (outFile != NULL) {
                outFD = open(outFile, O_RDWR | O_CREAT);
                fchmod(outFD, 0777); // allows read/write permissions for fileRedir (macOS specific issue I believe)
            }
        }
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        if (inFile != NULL) {
            dup2(inFD, STDIN_FILENO);
            close(inFD);
        }
        if (outFile != NULL) {
            dup2(outFD, STDOUT_FILENO);
            close(outFD);
        }
        execvp(argv[0], argv);
        perror("cannot execute command");
        exit(1);
    } else {
        if (wait(&child_info) == -1)
            perror("wait");
    }
    return child_info;
}


/**
 * Execute command to run if the cmdline contains a pipe
*/
int executePipe(char ***pipeCmds, int numCommands, char *inFiles[], char *outFiles[], const char redirPos[]) {
    pid_t pid; // process ID's
    int child_info = -1;
    int newPipe[2], prevPipe[2];
    int currCommand = 0;
    int inFD, outFD;

    // if there are less than 2 commands, there is no pipe
    if (numCommands < 2) {
        perror("Not enough commands to pipe");
        exit(1);
    }

    int i;
    for (i = 0; i < numCommands; i++) {
        /* Something has gone horrible wrong, the pipe is a lie
            and the promised reward was a fictitious motivator*/
        if (pipeCmds[i] == NULL) {
            perror("A pipe is null");
            exit(1);
        }
    }

    while (currCommand < numCommands) {
        // if this is not last command
        if (currCommand != numCommands - 1) {
            if (pipe(newPipe) == -1) {
                perror("Issue with pipe");
                exit(1);
            }
        }

        // fork
        if ((pid = fork()) == -1) {
            perror("Fork failed");
            exit(1);
        }

            // Parent Process
        else if (pid > 0) {
            // if this is not first command
            if (currCommand != 0) {
                close(prevPipe[0]);
                close(prevPipe[1]);
            }
            // if this is not final command
            if (currCommand != numCommands - 1) {
                prevPipe[0] = newPipe[0];
                prevPipe[1] = newPipe[1];
            }
            // wait for child
            if (wait(&child_info) == -1) {
                perror("wait issue");
            }
            currCommand++;
        }

            // Child Process
        else {
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);

            // not first command
            if (currCommand != 0) {
                close(prevPipe[1]); // close writing end
                dup2(prevPipe[0], 0); // redirect stdin
            }
            // not last command
            if (currCommand != numCommands - 1) {
                close(newPipe[0]); // close reading end
                dup2(newPipe[1], 1); // redirect stdout
            }

            if (redirPos != NULL) {
                if (redirPos[currCommand] != '\0') {
                    // if redirect is input
                    if (redirPos[currCommand] == '>') {
                        outFD = open(outFiles[currCommand], O_RDWR | O_CREAT);
                        fchmod(outFD, 0777);
                        dup2(outFD, STDOUT_FILENO);
                        dup2(newPipe[1], outFD);
                    }
                        // if redirect is output
                    else if (redirPos[currCommand] == '<') {
                        inFD = open(inFiles[currCommand], O_RDWR | O_CREAT);
                        fchmod(inFD, 0777);
                        dup2(inFD, STDIN_FILENO);
                        dup2(newPipe[0], inFD);
                    }
                }
            }
            // if curr command has redirect


            // execute
            execvp(pipeCmds[currCommand][0], pipeCmds[currCommand]);

            // if exec fail
            perror("Issue executing");
            exit(1);
        }
    }

    // cleanup
    close(newPipe[0]);
    close(newPipe[1]);
    close(prevPipe[0]);
    close(prevPipe[1]);

    return child_info;
}