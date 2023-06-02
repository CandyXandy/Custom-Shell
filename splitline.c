/* splitline.c - commmand reading and parsing functions for smsh
 *    
 *    char *next_cmd(char *prompt, FILE *fp) - get next command
 *    char **splitline(char *str);           - parse a string

 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"smsh.h"
#include    <glob.h>

char * next_cmd(char *prompt, FILE *fp)
/*
 * purpose: read next command line from fp
 * returns: dynamically allocated string holding command line
 *  errors: NULL at EOF (not really an error)
 *          calls fatal from emalloc()
 *   notes: allocates space in BUFSIZ chunks.  
 */
{
	char	*buf ; 				/* the buffer		*/
	int	bufspace = 0;			/* total size		*/
	int	pos = 0;			/* current position	*/
	int	c;				/* input char		*/
	int isPipe = 0;		// flag for pipe in command

	printf("%s", prompt);				/* prompt user	*/
	while( ( c = getc(fp)) != EOF ) {

		/* need space? */
		if( pos+1 >= bufspace ){		/* 1 for \0	*/
			if ( bufspace == 0 )		/* y: 1st time	*/
				buf = calloc(BUFSIZ, 1);

			else				/* or expand	*/
				buf = erealloc(buf,bufspace+BUFSIZ);

			bufspace += BUFSIZ;		/* update size	*/
		}

		if (c == '|')  // if there is a pipe, set isPipe to 1
			isPipe = 1;
			
		/* end of command? */
		if ( c == '\n' )
			break;

		/* no, add to buffer */
		buf[pos++] = c;

	}
	if ( c == EOF && pos == 0 )		/* EOF and no input	*/
		return NULL;			/* say so		*/

	buf[pos] = '\0';
	return buf;
}

/**
 **	splitline ( parse a line into an array of strings )
 **/
#define	is_delim(x) ((x)==' '||(x)=='\t' || (x)=='|' || (x) == '>' || (x) == '<')

char ** splitline(char *line)
/*
 * purpose: split a line into array of white-space separated tokens
 * returns: a NULL-terminated array of pointers to copies of the tokens
 *          or NULL if line if no tokens on the line
 *  action: traverse the array, locate strings, make copies
 *    note: strtok() could work, but we may want to add quotes later
 */
{
	char	*newstr(char *, int );
	char	**args ;
	int	spots = 0;			/* spots in table	*/
	int	bufspace = 0;			/* bytes in table	*/
	int	argnum = 0;			/* slots used		*/
	char	*cp = line;			/* pos in string	*/
	char	*start;
	int	len;

	if ( line == NULL )			/* handle special case	*/
		return NULL;

	args     = calloc(BUFSIZ, 1);		/* initialize array	*/
	bufspace = BUFSIZ;
	spots    = BUFSIZ/sizeof(char *);

	while( *cp != '\0' )
	{
		while ( is_delim(*cp) )		/* skip leading spaces	*/
			cp++;
		if ( *cp == '\0' )		/* quit at end-o-string	*/
			break;

		/* make sure the array has room (+1 for NULL) */
		if ( argnum+1 >= spots ){
			args = erealloc(args,bufspace+BUFSIZ);
			bufspace += BUFSIZ;
			spots += (BUFSIZ/sizeof(char *));
		}

		/* mark start, then find end of word */
		start = cp;
		len   = 1;
		while (*++cp != '\0' && !(is_delim(*cp)) )
			len++;
		args[argnum++] = strndup(start, len);
	}
	args[argnum] = NULL;
	return args;
}


#define intSize sizeof(int) // size of an int in bytes
/**
 * splitlinePipe ( parse a line into two arrays of strings, split by a pipe )
 * @param line - the line to be split
 * @param CommandList - the array of commands to be filled
 * @param numCommands - the number of commands in the line
 * @return void
*/
char *** splitlinePipe(char* line, int numCommands) {
	int * pipePositions; // positions of the pipe
	int lineLen = strlen(line); // length of the line

    char *** commandList = calloc(numCommands + 2, BUFSIZ); // ensure enough space for each individual command
	if (line == NULL) { // if the line is null, return null
		return NULL;
	}

	pipePositions = calloc((intSize * (numCommands + 1) ), intSize); // allocate memory for int array

	int pipeIndex = 0; // index of the current pipe
	// find the position of the pipe
    int i;
	for (i = 0; i < lineLen; i++) {
		if (line[i] == '|') {
			pipePositions[pipeIndex] = i; // add the position of the pipe to the array
			pipeIndex++; // increment the index
		}
	}
	pipePositions[pipeIndex] = lineLen; // add the length of the line to the array

	char * pipeString;
    pipeString = strndup(line, pipePositions[0]);
	// split the line into strings before and after each pipe
	for (i = 0; i < numCommands; i++) {
        commandList[i] = splitline(pipeString);
		int pipeStringLen = pipePositions[i+1] - pipePositions[i]; // length of the string between the pipes
		pipeString = strndup(line + pipePositions[i], pipeStringLen);

	}
    commandList[numCommands] = NULL;
	free(pipeString);
    free(pipePositions);
    return commandList;
}

/**
 * given a wildcard character, returns a list of path names in a single string
 * seperated by spaces
 * ref: https://stackoverflow.com/questions/36757641/how-to-use-the-glob-function
 * @param wildCard : the wildcard to use as our search term
 */
char * globPattern(char * wildCard) {
    int length = 0;
    int numMatch = 0;
    glob_t result;
    char ** list;

    int err = glob(wildCard, GLOB_ERR, NULL, &result);
    if (err != 0) {
        if (err == GLOB_NOMATCH) {
            perror("No matches found for your wildcard!");
        } else {
            perror("Issue with globbing");
        }
        return NULL;
    }

    // find how many strings we need space for
    for (list = result.gl_pathv, numMatch = result.gl_pathc;
        // gl_pathv is an array of matching pathnames //gl_pathc is the count of matches
            numMatch; list++, numMatch--) {
        length += strlen(*list) + 1;
        length += 2; // allow room for space characters
    }
    // use calloc here to ensure allocated buffer is initialised.
    char * pathListString = calloc(length, 1);

    // start appending the path names onto our return string
    for (list = result.gl_pathv, numMatch = result.gl_pathc;
            numMatch; list++, numMatch--) {
        strcat(pathListString, *list);
        if (numMatch > 1) { // add space between words
            strcat(pathListString, " ");
        }
    }

    // free the memory allocated for our glob struct
    globfree(&result);
    return pathListString;
}

/*
 * purpose: constructor for strings
 * returns: a string, never NULL
 */
char *newstr(char *s, int l)
{
	char *rv = emalloc(l+1);

	rv[l] = '\0';
	strncpy(rv, s, l);
	return rv;
}

void 
freelist(char **list)
/*
 * purpose: free the list returned by splitline
 * returns: nothing
 *  action: free all strings in list and then free the list
 *
 *  Sorry professor but valgrind made me change this in my
 *  obsessive hunt for memory leaks
 */
{
    int n;
    for (n = 0; list[n] != NULL; n++) {
        free(list[n]);
    }
    free(list);
//	char	**cp = list;
//	while( *cp )
//		free(*cp++);
//	free(list);
}

/**
 * free the 2d list returned by splitlinePipe
 */
void free2dlist(char *** list) {
    int i;
    for (i = 0; list[i] != NULL; i++) {
        int j;
        for (j = 0; list[i][j] != (void *)0; j++) {
            free(list[i][j]);
        }
        free(list[i]);
    }
    free(list);
}

void * emalloc(size_t n)
{
	void *rv ;
	if ( (rv = malloc(n)) == NULL )
		fatal("out of memory","",1);
	return rv;
}
void * erealloc(void *p, size_t n)
{
	void *rv;
	if ( (rv = realloc(p,n)) == NULL )
		fatal("realloc() failed","",1);
	return rv;
}
