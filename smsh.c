/* smsh -- simple command processor */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
// #include <wait.h>

#define EOL 1				/* end of line */
#define ARG 2				/* normal argument */
#define AMPERSAND 3
#define SEMICOLON 4
#define REDIRECT 5			/* cmd > file_name */
#define REDIRECT_FORCE 6
#define REDIRECT_APPEND 7
#define REDIRECT_IN 8
#define PIPE 9

#define MAXARG 1024			/* max. no. command args */
#define MAXBUF 1024			/* max. length input line */
#define MAXHIS 500

#define FOREGROUND 0
#define BACKGROUND 1

#define FPERM 0644

struct rdrct
{
	int in;
	int out;
	char *in_file_name;
	char *out_file_name;
};


char prompt[256];		/* prompt */
int count;				/* input count */
int sigint_flag = 0;

/* program buffers and work pointers */
char inpbuf[MAXBUF], tokbuf[2 * MAXBUF], *ptr = inpbuf, *tok = tokbuf;
char special[] = {' ', '\t', '&', ';', '\n', '>', '<', '|', '\0'};
int history_fd, history_begin, history_end, history_index;
char *history[MAXHIS];

int userin(char *p);
int gettok(char **outptr);
int inarg(char c); 	/* are we in an ordinary argument */
void procline(void);	/* process input lines */
int runcommand(char ***pipes, int isBack, struct rdrct* redirect, int pipecnt);
void setprompt(void);
int change_directory(char **cline);
void open_history_file(void);
void add_command_into_history(char *cmd);
void show_history(void);
void signal_handler(int signo)
{
	sigint_flag = 1;
}

static struct sigaction act, oact;
static sigset_t set1, set2;

int main(void)
{
	/* sigaction for SIGINT */
	sigaction(SIGINT, NULL, &oact);
	act.sa_handler = signal_handler;
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	sigfillset (&set1);
	sigemptyset(&set2);
	sigdelset(&set1, SIGINT);
	
	/* Block other signals */
	sigprocmask(SIG_SETMASK, &set1, NULL);
	open_history_file();
	setprompt();
	while (userin(prompt) != EOF)
	{	
		sigfillset (&set1);
		sigprocmask(SIG_SETMASK, &set1, NULL);

		add_command_into_history(ptr);
		procline();

		sigdelset(&set1, SIGINT);
		sigprocmask(SIG_SETMASK, &set1, NULL);
	}
}

void show_history(void)
{
	int j = 1;
	for (int i = history_begin; i != history_end; i++, j++)
	{
		printf("%3d\t%s", (history_index - MAXHIS + j < 1 ? j : history_index - MAXHIS + j), history[i]);
	}
}

void add_command_into_history(char *cmd)
{
	int blnk = 1;
	/* blank check */
	for (int i = 0; cmd[i]; i++)
		if(cmd[i] != '\n' && cmd[i] != ' ' && cmd[i] != '\t')
		{
			blnk = 0;
			break;
		}
	
	if(blnk)
		return;
		
	if (write(history_fd, cmd, strlen(cmd)) < 0)
	{
		perror("write error");
		exit(1);
	}
	char *cp;
	cp = malloc(sizeof(MAXBUF) * sizeof(char));
	strcpy(cp, cmd);
	history[history_end++] = cp;
	free(history[history_end]);
	
	if ((history_end %= MAXHIS) == history_begin)
		history_begin = (history_begin + 1) % MAXHIS;
	history_index++;
}

void open_history_file(void)
{
	uid_t user_id;
	struct passwd *user_pw;

	user_id  = getuid();			// 사용자 아이디를 구하고
    user_pw  = getpwuid(user_id);	// 아이디로 사용자 정보 구하기

	char path[256];
	sprintf(path, "%s/%s", user_pw->pw_dir, "/.smsh_history");

	history_index = 0;
	int history_perm = 0600;
	history_begin = 0;
	history_end = history_begin = 0; 
	history_fd = open(path, O_CREAT|O_RDWR, history_perm);

	FILE *file = fdopen(history_fd, "r");
	char buf[1024], *cp;
	while (fgets(buf, sizeof(buf), file))
	{
		cp = malloc(sizeof(buf));
		strcpy(cp, buf);
		history[history_end++] = cp;
		if ((history_end %= MAXHIS) == history_begin)
			history_begin = (history_begin + 1) % MAXHIS;
		history_index++;
	}

	return;
}

int userin(char *p)
{
	int c, history_cursor = history_end;

	/* initialization for later routines */
	ptr = inpbuf;
	tok = tokbuf;

	/* display prompt */
	printf("%s ", p);

	for (count = 0, sigint_flag = 0;;sigint_flag = 0) {
		if ((c = getchar()) == EOF && sigint_flag == 0)
		{
			printf("\nBye\n");
			return EOF;
		}

		if (sigint_flag == 1)
		{
			count = 0;
			printf("\n%s ", prompt);
			continue;
		}
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
		if (*ptr == '|')
		{
			ptr++;
			type = REDIRECT_FORCE;
		}
		else if (*ptr == '>')
		{
			ptr++;
			type = REDIRECT_APPEND;
		}
		else
		{
			type = REDIRECT;
		}
		break;
	case '<':
		type = REDIRECT_IN;
		break;
	case '|':
		type = PIPE;
		break;
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
	char **pipes[MAXARG + 1];	/* seperate commands in arg */
	int pipecnt;				/* number of pipes */

	red.in = 0;
	red.out = 0;
	red.in_file_name = NULL;
	red.out_file_name = NULL;
	pipecnt = 0;
	pipes[0] = arg;

	for (narg = 0;;) {	/* loop FOREVER */

		/* take action according to token type */
		switch (toktype = gettok(&arg[narg])) {
		case ARG:
			if (red.in != 0 && red.in_file_name == NULL)
			{
				red.in_file_name = arg[narg];
			}
			else if (red.out != 0 && red.out_file_name == NULL)
			{
				red.out_file_name = arg[narg];
			}
			else
			{
				if (narg < MAXARG)
				narg++;
			}
			break;
		case REDIRECT:
			red.out = REDIRECT;
			break;
		case REDIRECT_FORCE:
			red.out = REDIRECT_FORCE;
			break;
		case REDIRECT_APPEND:
			red.out = REDIRECT_APPEND;
			break;
		case REDIRECT_IN:
			red.in = REDIRECT_IN;
			break;
		case PIPE:
			arg[narg++] = NULL;
			pipes[++pipecnt] = &arg[narg];
			break;
		case EOL:
		case SEMICOLON:
		case AMPERSAND:

			amp = (toktype == AMPERSAND) ? BACKGROUND : FOREGROUND;

			if (narg != 0) {
				arg[narg] = NULL;
				runcommand(pipes, amp, &red, pipecnt);
			}	

			if (toktype == EOL)
				return;
			
			narg = 0;
			break;
		}
	}
}

int runcommand(char ***pipes, int isBack, struct rdrct* redirect, int pipecnt)
{
	int pid, exitstat, ret, cnt = 0;
	int in_fd, out_fd;

	/* if change directory command */
	if (strcmp(*pipes[0], "cd") == 0)
	{
		ret = change_directory(pipes[0]);
		setprompt();
		return ret;
	}

	if ((pid = fork()) < 0) {
		perror("smsh");
		return -1;
	}

	if (pid == 0) { /* child */
		sigaction(SIGINT, &oact, NULL);
		sigprocmask(SIG_SETMASK, &set2, NULL);
		/* redirect */
		if (redirect->in != 0)
		{
			in_fd = open(redirect->in_file_name, O_RDONLY);
			if (in_fd < 0)
			{
				fprintf(stderr, "%s: No such file or directory\n", redirect->in_file_name);
				return -1;
			}
			close(STDIN_FILENO);
			dup(in_fd);
			close(in_fd);
			/* stdin is now redirected */
		}
		if (redirect->out != 0)
		{
			int flags = O_CREAT|O_WRONLY;
			switch (redirect->out) {
				case REDIRECT_APPEND:
					flags |= O_APPEND; break;
				case REDIRECT_FORCE:
					flags |= O_TRUNC; break;
				default:
					flags |= O_EXCL; break;
					break;
			}
			out_fd = open(redirect->out_file_name, flags, FPERM);
			if (out_fd < 0)
			{
				fprintf(stderr, "%s: cannot overwrite existing file\n", redirect->out_file_name);
				return -1;
			}
			close(STDOUT_FILENO);
			dup(out_fd);
			close(out_fd);
			/* stdout is now redirected */
		}
		/* piping */
		if (pipecnt > 0) {
			int files[pipecnt][2];
			for (int i = 0; i < pipecnt; i++)
				pipe(files[i]);
			
			for (cnt = 0; cnt < pipecnt; cnt++)
			{
				if (fork() == 0) {
					
					close(STDOUT_FILENO);
					dup(files[cnt][1]);

					if (cnt != 0)
					{
						close(STDIN_FILENO);
						dup(files[cnt - 1][0]);
					}

					for (int i = 0; i < pipecnt; i++)
					{
						close(files[i][0]);
						close(files[i][1]);
					}

					execvp(*pipes[cnt], pipes[cnt]);
					perror(*pipes[cnt]);
					exit(127);
				}
			}
			close(STDIN_FILENO);
			dup(files[cnt - 1][0]);
			for (int i = 0; i < pipecnt; i++)
			{
				close(files[i][0]);
				close(files[i][1]);
			}
		}

		if (strcmp(*pipes[0], "history") == 0)
		{
			show_history();
			exit(127);
		}
		else
		{
			execvp(*pipes[cnt], pipes[cnt]);
			perror(*pipes[cnt]);
			exit(127);
		}
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
		if (chdir(cline[1]) < 0) {
			perror("cd");
			return -1;
		}
		return 0;
	}
}
