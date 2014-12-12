
all: smtpclient

objects: smtpclient_main.o

smtpclient: Makefile smtpclient_main.c
	$(Q)$(CC) -o smtpclient smtpclient_main.c

smtpclient_main.o: Makefile smtpclient_main.c
	$(Q)$(CC) -c $(CFLAGS) smtpclient_main.c

clean:
	$(Q)rm -f *.o *.gdb *.elf *~ smtpclient

.PHONY: all install clean
