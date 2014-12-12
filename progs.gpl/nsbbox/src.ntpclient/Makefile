
CFLAGS+= -I../../../include

all: ntpclient
objects: ntpclient.o phaselock.o

ntpclient: ntpclient.o phaselock.o
	$(Q)$(CC) -o ntpclient ntpclient.o phaselock.o

ntpclient.o: Makefile ntpclient.c
	$(Q)$(CC) -c $(CFLAGS) ntpclient.c

phaselock.o: Makefile phaselock.c
	$(Q)$(CC) -c $(CFLAGS) phaselock.c

clean:
	$(Q)rm -f *.o *.gdb *.elf ntpclient

.PHONY: all fresetd clean
