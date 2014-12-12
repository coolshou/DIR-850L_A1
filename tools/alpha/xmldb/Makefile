
ARCH_CONFIG=../config.arch
-include $(ARCH_CONFIG)

#CFLAGS+=-DDDEBUG
#CFLAGS+=-DDUMP_SBLIST
OBJS=main.o xmldb.o patch.o dtrace.o dirutil.o ephp.o
APPLET=xmldb

# Common make rules
%.o: %.c
	$(CC) $(CFLAGS) -c $<


all: $(APPLET)

$(APPLET): Makefile $(OBJS)
	$(CC) $(LDFLAGS) -o $(APPLET) $(OBJS)

clean:
	rm -f *~ *.o *.elf *.gdb $(APPLET)

.PHONY: all install clean
