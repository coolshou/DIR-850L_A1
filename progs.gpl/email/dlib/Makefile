# Dlib makefile
-include ../../arch.mk
-include ../../../path.mk
-include $(TOPDIR)/.config
-include /tmp/$(USER)/$(shell echo $(ELBOX_MODEL_NAME))_path.mk

MAKE = make
VERSION = @ver@
SRCDIR = src/
DESTDIR = 
EMAILPATH = $(TOPDIR)/progs.gpl/email
CFLAGS += -g -O2 -Wall -W -I$(OPENSSLDIR)/include -I$(EMAILPATH)/dlib -I$(EMAILPATH)/dlib/src -I$(EMAILPATH)/dlib/include -I../include  -DHAVE_CONFIG_H
ifeq ($(strip $(ELBOX_PROGS_PRIV_CYASSL)),y)
CFLAGS += -DUSE_CYASSL
endif
LDFLAGS += -L$(OPENSSLDIR) 
LIBS = -lcrypto -lssl -lnsl -lm 

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
mandir = ${datarootdir}/man
docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}
sysconfdir = ${prefix}/etc
datarootdir = ${prefix}/share



bin_suffix = 

FILES = src/dnet.o src/dhash.o src/dlist.o src/dstrbuf.o src/dutil.o src/dvector.o

.PHONY: all clean-all clean distclean install uninstall

all: $(FILES)
	ar rc libdlib.a $(FILES)

test:
	$(CC) $(CFLAGS) -o test test.c libdlib.a -lm

dlisttest:
	$(CC) $(CFLAGS) -o dlisttest dlisttest.c libdlib.a -lm

dhashtest:
	$(CC) $(CFLAGS) -o dhashtest dhashtest.c libdlib.a -lm

clean:
	rm -f src/*.o *.so *.a test

