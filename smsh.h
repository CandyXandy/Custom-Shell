#define	YES	1
#define	NO	0

char	*next_cmd(char *, FILE *);
char    *globPattern(char * );
char	**splitline(char *);
char    ***splitlinePipe(char *, int);
void	freelist(char **);
void    free2dlist(char ***);
void	*emalloc(size_t);
void	*erealloc(void *, size_t );
int	    execute(char **, char *, char *);
int	    executePipe(char ***, int , char **, char **, const char *);
void	fatal(char *, char *, int );

int	process();
