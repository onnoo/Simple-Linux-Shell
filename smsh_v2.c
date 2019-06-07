/*
 * smsh - ver 2
 * author : onnoo
 * date : 2019-06-07
 * github : https://github.com/onnoo/Simple-Linux-Shell
 */

#include <stdio.h>

int user_input(void);

int main(void) {

	/* read command line until "end of file" */

	while (read(stdin, buffers, numberchars)) {
		/* parse command line */
		if (/* command line contains &*/)
			amp = 1;
		else
			amp = 0;

		if (fork() == 0) {
			if (/* redirect output */) {
				fd = creat(newfile, fmask);
				close(stdout);
				dup(fd);
				close(fd);
				/* stdout is now redirected */
			}
			if (/* piping */) {
				pipe(files);
				if (fork() == 0) {
					/* first component of command line */
					close(stdout);
					dup(files[1]);
					close(files[1]);
					close(files[0]);
					/* stdout now goes to pipe */
					/* child process does command1 */
					execvp(command1, args1);
				}
				/* 2nd command of command line */
				close(stdin);
				dup(files[0]);
				close(files[0]);
				close(files[1]);
				/* stdin now comes from pipe */
			}
			execvp(command2, args2);
		}
		/* parent continues over here,
		 * wait for child to exit if required
		 */
		if (amper == 0)
			reid = wait(&status);
	} // end of while loop

	return 0;
}
