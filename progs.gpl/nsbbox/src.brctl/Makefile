
all: brctl
objects: brctl.o brctl_cmd.o brctl_disp.o libbridge/libbridge.a

CFLAGS+= -I./libbridge

brctl: brctl.o brctl_cmd.o brctl_disp.o libbridge/libbridge.a
	$(Q)$(CC) -Wall -g $(LDFLAGS) -o brctl brctl.o brctl_cmd.o brctl_disp.o libbridge/libbridge.a

clean:
	$(Q)make -C libbridge clean
	$(Q)rm -f *.o *.gdb *.elf brctl core

brctl.o: Makefile brctl.c brctl.h
	$(Q)$(CC) -c $(CFLAGS) brctl.c

brctl_cmd.o: Makefile brctl_cmd.c brctl.h
	$(Q)$(CC) -c $(CFLAGS) brctl_cmd.c

brctl_disp.o: Makefile brctl_disp.c brctl.h
	$(Q)$(CC) -c $(CFLAGS) brctl_disp.c

libbridge/libbridge.a:
	$(Q)make -C libbridge

install:
	$(Q)install brctl $(TARGET)/usr/sbin

.PHONY: all objects install clean
