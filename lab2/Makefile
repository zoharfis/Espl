all: Looper myshell mypipe

myshell: LineParser.o myshell.o
	gcc -g -m32 -Wall -o myshell LineParser.o myshell.o 

mypipe: mypipe.o
	gcc -g -m32 -Wall -o mypipe mypipe.o

Looper: Looper.o
	gcc -g -m32 -Wall -o Looper Looper.o

LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c

myshell.o: myshell.c
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

mypipe.o: mypipe.c
	gcc -m32 -g -Wall -c -o mypipe.o mypipe.c

Looper.o: Looper.c
	gcc -m32 -g -Wall -c -o Looper.o Looper.c

.PHONY: clean 

clean:
	rm -f *.o Looper myshell mypipe