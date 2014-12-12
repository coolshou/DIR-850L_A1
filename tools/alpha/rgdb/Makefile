
all: rgdb

rgdb: Makefile rgdb.c lrgbin.c
	cc -c lrgbin.c
	cc -c rgdb.c
	cc -o rgdb rgdb.o lrgbin.o

clean:
	rm -f *.o *.gdb *~ *.elf rgdb

.PHONY: all clean
