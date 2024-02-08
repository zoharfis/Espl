all: Looper mypipeline myshell

myshell: LineParser.o myshell.o
	gcc -g -m32 -Wall -o myshell LineParser.o myshell.o 

mypipeline: mypipeline.o
	gcc -g -m32 -Wall -o mypipeline mypipeline.o

Looper: Looper.o
	gcc -g -m32 -Wall -o Looper Looper.o

LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c

myshell.o: myshell.c
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

mypipeline.o: mypipeline.c
	gcc -m32 -g -Wall -c -o mypipeline.o mypipeline.c

Looper.o: Looper.c
	gcc -m32 -g -Wall -c -o Looper.o Looper.c

.PHONY: clean 

clean:
	rm -f *.o Looper mypipeline myshell