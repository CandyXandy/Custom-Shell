/**
 * Alexander Robertson
 * 20/05/2023
 *
 * smsh4.c small-shell version 4
 *     This version of shell performs all of the
 *     shell operations of smsh3, but also allows
 *     globbing in comands
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "smsh.h"

#define DFL_PROMPT "> "
#define MAX_PIPE 20

int main() {
    // initialise strings
    char *cmdline = "", *prompt, **arglist;
    // these are for the event of a pipe
    char ***pipes;
    int doPipe = 0;
    int doInputRedir = 0;
    int doOutputRedir = 0;

    char redirPos[MAX_PIPE] = {'\0'}; // hold what redir type at numCommand index
    // if we have multiple redirects on the same command we need to decide which is first
    int inFirst = 0;
    int outFirst = 0;

    int result;
    void setup();

    prompt = DFL_PROMPT;
    setup();

    while ((cmdline = next_cmd(prompt, stdin)) != NULL) {
        int numCommands = 1;
        int i;
        for (i = 0; i < strlen(cmdline); i++) {
            if (cmdline[i] == '|') {
                doPipe = 1;
                numCommands++;
            }
            if (cmdline[i] == '<') {
                doInputRedir = 1;
                // same cmd multi-redirect is only supported for no pipes in cmd
                if (!outFirst || numCommands > 1) {
                    redirPos[numCommands - 1] = '<';
                    if (numCommands == 1)
                        inFirst = 1;
                } else  {
                    redirPos[numCommands] = '<';
                    inFirst = 0;
                }
            }
            if (cmdline[i] == '>') {
                doOutputRedir = 1;
                if (!inFirst || numCommands > 1) {
                    redirPos[numCommands - 1] = '>';
                    if (numCommands == 1){
                        outFirst = 1;
                    }
                } else {
                    redirPos[numCommands] = '>';
                    outFirst = 0;
                }
            }
            if (cmdline[i] == '*' || cmdline[i] == '?' || cmdline[i] == '[') {
                size_t cmdlen = strlen(cmdline);
                char *wildcard = malloc(cmdlen); // create string to use as a wildcard check
                int wildClen = 0;
                int t = i; // so we may go back to where we were in for loop

                // go back to start of wildcard word
                while (cmdline[i] != ' ' && cmdline[i] != '\"') {
                    i--;
                }
                int wildCStart = i + 1;
                while (cmdline[i] == ' ') i++; // step forward once to ensure we're not still on space
                while (cmdline[i] != ' ' && cmdline[i] != '\0') { // build wildcard string
                    wildcard[wildClen] = cmdline[i];
                    wildClen++;
                    i++;
                }
                wildcard[wildClen] = '\0'; // null terminate the wildcard string
                char *glob = globPattern(wildcard); // generate a string of matches

                // create strings up until wildcard and after the wildcard
                char *beforeGlob = strndup(cmdline, wildCStart);
                char *afterGlob = strndup(cmdline + wildCStart + wildClen, (cmdlen - (wildCStart + wildClen)));

                // start concatenating the pieces, ensuring to realloc enough memory at each step
                char *newCmdline = strndup(beforeGlob, strlen(beforeGlob));
                if (glob != NULL) {
                    newCmdline = erealloc(newCmdline, strlen(newCmdline) + strlen(glob) + 1);
                    strcat(newCmdline, glob);
                    i = t + (strlen(glob));
                }
                newCmdline = erealloc(newCmdline, strlen(newCmdline) + strlen(afterGlob) + 1);
                strcat(newCmdline, afterGlob);
                newCmdline = erealloc(newCmdline, strlen(newCmdline) + 1);
                strcat(newCmdline, "\0");
                // make cmdline a duplicate of our newcmdline string
                cmdline = strndup(newCmdline, strlen(newCmdline));

                // cleanup
                free(newCmdline);
                free(glob);
                free(beforeGlob);
                free(afterGlob);
                free(wildcard);
            }
        }
        if (strcmp(cmdline, "exit") == 0) {
            return 0;
        }
        if (doPipe) {
            // split the command line into as many pipes as there are
            pipes = splitlinePipe(cmdline, numCommands);
            int k;
            char *outFiles[numCommands]; // holds files for input/output redirection
            char *inFiles[numCommands];
            for (k = 0; pipes[k] != NULL; k++) {
                int j;
                outFiles[k] = NULL; // initialise all values with NULL
                inFiles[k] = NULL; // avoid free errors when we want to free the list
                for (j = 0; pipes[k][j] != NULL; j++) {
                    if ((strchr(pipes[k][j], '.') != NULL)) {

                        if (doOutputRedir) {
                            if (redirPos[k] == '>') {
                                outFiles[k] = strndup(pipes[k][j], strlen(pipes[k][j]));
                                pipes[k][j] = NULL;
                            }

                        }
                        if (doInputRedir) {
                            if (redirPos[k] == '<') {
                                inFiles[k] = strndup(pipes[k][j], strlen(pipes[k][j]));
                                pipes[k][j] = NULL;
                            }

                        }
                    }
                }

            }
            // execute the commands
            result = executePipe(pipes, numCommands, inFiles, outFiles, redirPos);

            // cleanup for next cmdLine
            free2dlist(pipes);
            if (doOutputRedir) {
                int m;
                for (m = 0; m < numCommands; m++) {
                    if (outFiles[m] != NULL) {
                        free(outFiles[m]);
                    }
                }
            }
            if (doInputRedir) {
                int m;
                for (m = 0; m < numCommands; m++) {
                    if (inFiles[m] != NULL) {
                        free(inFiles[m]);
                    }
                }
            }

        } else if ((arglist = splitline(cmdline)) != NULL) {
            int k;
            char *inFile = NULL;
            char *outFile = NULL;
            for (k = 0; arglist[k] != NULL; k++) {
                if ((strchr(arglist[k], '.') != NULL)) { // if we see a '.' we probably are dealing with redir
                    if (doOutputRedir || doInputRedir) { // but not necessarily so double check (ie rm word.txt)
                        if (doInputRedir) {
                            if (outFirst) {
                                outFirst = 0;
                                continue;
                            }
                            inFile = strndup(arglist[k], strlen(arglist[k]));
                        }
                        if (doOutputRedir) {
                            if (inFirst) {
                                inFirst = 0;
                                continue;
                            }
                            outFile = strndup(arglist[k], strlen(arglist[k]));
                        }
                        arglist[k] = NULL; // replace the command with null terminator as it's not an actual command
                    }
                }
            }
            result = execute(arglist, inFile, outFile);
            // cleanup for next cmdLine
            freelist(arglist);
            free(inFile);
            free(outFile);
        }
        // cleanup for next cmdLine
        free(cmdline);
        doPipe = 0;
        doOutputRedir = 0;
        doInputRedir = 0;
        inFirst = 0;
        outFirst = 0;
    }

    return 0;
}


/**
 * initialises shell
*/
void setup() {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
}

void fatal(char *s1, char *s2, int n) {
    fprintf(stderr, "Error: %s,%s\n", s1, s2);
    exit(n);
}