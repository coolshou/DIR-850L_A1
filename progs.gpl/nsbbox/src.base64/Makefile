
all: base64
objects: base64.o

base64: Makefile base64.o
	$(Q)$(CC) -o base64 base64.o

base64.o: Makefile base64.c
	$(Q)$(CC) -c $(CFLAGS) base64.c

clean:
	$(Q)rm -f *.o rgdb *.gdb *.elf

.PHONY: all objects clean
