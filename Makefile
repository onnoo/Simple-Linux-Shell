smsh: smsh.o
	gcc -o smsh smsh.o

smsh.o: smsh.c
	gcc -c -o smsh.o smsh.c

clean:
	rm -f smsh.o