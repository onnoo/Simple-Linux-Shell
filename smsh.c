/* smsh -- simple command processor */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define EOL 1			/* end of line */
#define ARG 2			/* normal argument */
#define AMPERSAND 3
#define SEMICOLON 4

#define MAXARG 1024		/* max. no. command args */
#define MAXBUF 1024		/* max. length input line */

#define FOREGROUND 0
#define BACKGROUND 1

char *prompt = "Command>";		/* promprt */

/* program buffers and work pointers */
char inpbuf[MAXBUF], tokbuf[2 * MAXBUF], *ptr = inpbuf, *tok = tokbuf;
char special[] = {' ', '\t', '&', ';', '\n', '\0'};

int userin(char *p);
int gettok(char **outptr);
int inarg(char c); 	/* are we in an ordinary argument */
void procline(void);	/* process input lines */
int runcommand(char **cline, int where);

int main(void)
{
	while (userin(prompt) != EOF)
	{
		procline();
	}
}

int userin(char *p)
{
	int c, count;

	/* initialization for later routines */
	ptr = inpbuf;
	tok = tokbuf;

	/* display prompt */
	printf("%s ", p);

	for (count = 0;;) {
		if ((c = getchar()) == EOF)
			return EOF;
		
		if (count < MAXBUF)
			inpbuf[count++] = c;
		
		if (c == '\n' && count < MAXBUF) {
			inpbuf[count] = '\0';
			return count;
		}

		/* if line too long restsart */
		if (c == '\n') {
			printf("smsh:input line too long\n");
			count = 0;
			printf("%s ", p);
		}
	}
}

int gettok(char **outptr)
{
	int type;

	*outptr = tok;

	/* strip white space */
	for(;*ptr == ' ' || *ptr == '\t'; ptr++)
		;
	
	*tok++ = *ptr;

	switch (*ptr++) {
	case '\n':
		type = EOL; break;
	case '&':
		type = AMPERSAND; break;
	case ';':
		type = SEMICOLON; break;
	default:
		type = ARG;
		while(inarg(*ptr))
			*tok++ = *ptr++;
	}

	*tok++ = '\0';
	return type;
}

int inarg(char c)
{
	char *wrk;
	for (wrk = special; *wrk != '\0'; wrk++)
		if (c == *wrk)
			return 0;
	
	return 1;
}

void procline(void)
{
	char *arg[MAXARG + 1];		/* pointer array for runcommand */
	int toktype;				/* type of token in commnad */
	int narg;					/* number of arguments so far */
	int type;					/* FOREGROUND or BACKGROUND */

	for (narg = 0;;) {	/* loop FOREVER */

		/* take action according to token type */
		switch (toktype = gettok(&arg[narg])) {
		case ARG:
			if (narg < MAXARG)
				narg++;
			break;

		case EOL:
		case SEMICOLON:
		case AMPERSAND:

			type = (toktype == AMPERSAND) ? BACKGROUND : FOREGROUND;

			if (narg != 0) {
				arg[narg] = NULL;
				runcommand(arg, type);
			}	

			if (toktype == EOL)
				return;
			
			narg = 0;
			break;
		}
	}
}

int runcommand(char **cline, int where)
{
	int pid, exitstat, ret;

	if ((pid = fork()) < 0) {
		perror("smsh");
		return -1;
	}

	if (pid == 0) { /* child */
		execvp(*cline, cline);
		perror(*cline);
		exit(127);
	}

	/* code for parent */
	/* if background process print pid and exit */

	if (where == BACKGROUND) {
		printf("[Process id %d]\n", pid);
		return 0;
	}

	/* wait until process pid exits */

	while ((ret = wait(&exitstat)) != pid && ret != -1)
		;
	
	return (ret == -1 ? -1 : exitstat);
}