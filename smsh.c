/* smsh -- simple command processor */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>

#define EOL 1				/* end of line */
#define ARG 2				/* normal argument */
#define AMPERSAND 3
#define SEMICOLON 4
#define REDIRECT 5			/* cmd > file_name */

#define MAXARG 1024			/* max. no. command args */
#define MAXBUF 1024			/* max. length input line */

#define FOREGROUND 0
#define BACKGROUND 1

#define FPERM 0644

struct rdrct
{
	int type;
	char *file_name;
};


char prompt[256];		/* prompt */

/* program buffers and work pointers */
char inpbuf[MAXBUF], tokbuf[2 * MAXBUF], *ptr = inpbuf, *tok = tokbuf;
char special[] = {' ', '\t', '&', ';', '\n', '>', '\0'};

int userin(char *p);
int gettok(char **outptr);
int inarg(char c); 	/* are we in an ordinary argument */
void procline(void);	/* process input lines */
int runcommand(char **cline, int isBack, struct rdrct* redirect);
void setprompt(void);
int change_directory(char **cline);

int main(void)
{
	setprompt();
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

/* specify tokbuf */
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
	case '>':
		type = REDIRECT; break;
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
	int amp;					/* FOREGROUND or BACKGROUND */
	struct rdrct red;			/* REDIRECT KEYWORD */

	red.type = 0;

	for (narg = 0;;) {	/* loop FOREVER */

		/* take action according to token type */
		switch (toktype = gettok(&arg[narg])) {
		case ARG:
			if (red.type != 0)
			{
				red.file_name = arg[narg];
			}
			else
			{
				if (narg < MAXARG)
				narg++;
			}
			break;
		case REDIRECT:
			red.type = 1;
			break;
		case EOL:
		case SEMICOLON:
		case AMPERSAND:

			amp = (toktype == AMPERSAND) ? BACKGROUND : FOREGROUND;

			if (narg != 0) {
				arg[narg] = NULL;
				if (red.type == 0) {
					runcommand(arg, amp, NULL);
				} else {
					runcommand(arg, amp, &red);
				}
				
			}	

			if (toktype == EOL)
				return;
			
			narg = 0;
			break;
		}
	}
}

int runcommand(char **cline, int isBack, struct rdrct* redirect)
{
	int pid, exitstat, ret, fd;

	/* if change directory command */
	if (strcmp(cline[0], "cd") == 0)
	{
		ret = change_directory(cline);
		setprompt();
		return ret;
	}

	if ((pid = fork()) < 0) {
		perror("smsh");
		return -1;
	}

	if (pid == 0) { /* child */
		if (redirect != NULL){
			fd = creat(redirect->file_name, FPERM);
			close(STDOUT_FILENO);
			dup(fd);
			close(fd);
			/* stdout is now redirected */
		}
		execvp(*cline, cline);
		perror(*cline);
		exit(127);
	}

	/* code for parent */
	/* if background process print pid and exit */

	if (isBack == BACKGROUND) {
		printf("[Process id %d]\n", pid);
		return 0;
	}

	/* wait until process pid exits */

	while ((ret = wait(&exitstat)) != pid && ret != -1)
		;
	
	return (ret == -1 ? -1 : exitstat);
}

void setprompt(void)
{
	char cwd[256];
	uid_t user_id;
	struct passwd *user_pw;

	user_id  = getuid();			// 사용자 아이디를 구하고
    user_pw  = getpwuid(user_id);	// 아이디로 사용자 정보 구하기

	/* current woriking directory */
	getcwd(cwd, sizeof(cwd));
	if (strcmp(cwd, user_pw->pw_dir) >= 0)
	{
		prompt[0] = '~';
		strcpy(prompt + 1, cwd + strlen(user_pw->pw_dir));
	}
	else
	{
		strcpy(prompt, cwd);
	}

	sprintf(prompt, "%s$", prompt);
}

int change_directory(char **cline)
{
	if (cline[1] == '\0')
	{
		uid_t user_id;
		struct passwd *user_pw;

		user_id  = getuid();			// 사용자 아이디를 구하고
    	user_pw  = getpwuid(user_id);	// 아이디로 사용자 정보 구하기
		return chdir(user_pw->pw_dir);
	}
	else if (cline[2] != '\0')
	{
		fprintf(stderr, "cd: too many arguments\n");
		return -1;
	}
	else
	{
		return chdir(cline[1]);
	}
}