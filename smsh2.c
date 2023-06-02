/**
 * Alexander Robertson
 * 20/05/2023
 * 
 * smsh2.c small-shell version 2
 *     This version of shell performs all of the
 *     shell operations of smsh1, but also allows
 *     commands to be piped.
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
    char *cmdline, *prompt, **arglist;
    // these are for the event of a pipe
    char ***pipes;

    int doPipe = 0;

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
        }
        if (doPipe) {
            // split the command line into as many pipes as there are
            pipes = splitlinePipe(cmdline, numCommands); // pass references so we can mutate
            // execute the commands
            result = executePipe(pipes, numCommands, NULL, NULL, NULL);
            free2dlist(pipes);
        } else if ((arglist = splitline(cmdline)) != NULL) {
            result = execute(arglist, NULL, NULL);
            freelist(arglist);
        }
        free(cmdline);
        doPipe = 0;
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