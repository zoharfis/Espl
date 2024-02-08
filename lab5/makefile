all: task2

task2: task2.o startup.o start.o
	ld -o task2 task2.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

task2.o: task2.c
	gcc -m32 -c task2.c -o task2.o 

start.o: start.s
	nasm -f elf32 start.s -o start.o

startup.o: startup.s
	nasm -f elf32 startup.s -o startup.o

.PHONY: clean 

clean:
	rm -f *.o task2