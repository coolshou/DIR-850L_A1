
-include ../../../../path.mk
-include ../../../arch.mk

CFLAGS  += -DNSBBOX -I$(TOPDIR)/include

all: libbridge.a

clean:
	#rcsclean *
	$(Q)rm -f *~
	$(Q)rm -f *.o
	$(Q)rm -f libbridge.a

libbridge.a: libbridge_compat.o libbridge_devif.o libbridge_if.o libbridge_init.o libbridge_misc.o
	$(Q)ar rcs libbridge.a libbridge_compat.o libbridge_devif.o libbridge_if.o libbridge_init.o libbridge_misc.o
	$(Q)ranlib libbridge.a

libbridge_compat.o: libbridge_compat.c if_index.c
	$(Q)$(CC) $(CFLAGS) -c libbridge_compat.c

libbridge_devif.o: libbridge_devif.c libbridge.h libbridge_private.h
	$(Q)$(CC) $(CFLAGS) -c libbridge_devif.c

libbridge_if.o: libbridge_if.c libbridge.h libbridge_private.h
	$(Q)$(CC) $(CFLAGS) -c libbridge_if.c

libbridge_init.o: libbridge_init.c libbridge.h libbridge_private.h
	$(Q)$(CC) $(CFLAGS) -c libbridge_init.c

libbridge_misc.o: libbridge_misc.c libbridge.h libbridge_private.h
	$(Q)$(CC) $(CFLAGS) -c libbridge_misc.c

.PHONY: all clean
