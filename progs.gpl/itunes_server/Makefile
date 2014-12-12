-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config

HOST=$(HOST_TYPE)
pwd = $(shell pwd)
#MTDAAPDDIR=mt-daapd-svn-1696
MTDAAPDDIR=mt-daapd-0.2.4

ifeq ($(MTDAAPDDIR),mt-daapd-svn-1696)
SQLITE3_INCS = $(TOPDIR)/progs.priv/upnp_av_server/sqlite-3.5.8
SQLITE3_LIBS = $(TOPDIR)/progs.priv/upnp_av_server/sqlite-3.5.8
#ifeq ($(strip $(ELBOX_PROGS_GPL_SQLITE_374)),y)
#SQLITE3_INCS = $(TOPDIR)/progs.gpl/sqlite-3.7.4/build/sqlite-autoconf-3070400
#SQLITE3_LIBS = $(TOPDIR)/progs.gpl/sqlite-3.7.4/rootfs/lib
CONFIG_SQLITE = --enable-sqlite3 --with-sqlite3-includes=$(SQLITE3_INCS) \
    --with-sqlite3-libs=$(SQLITE3_LIBS)/.libs
#endif
else
SQLITE3_INCS=.
SQLITE3_LIBS=.
endif

all: zlib gdbm id3tag mtdaapd mkend  

sqlite:
	make -C $(TOPDIR)/progs.priv/upnp_av_server/ sqlite

zlib:
	make -C zlib-1.2.1/
	cp zlib-1.2.1/zlib.h $(MTDAAPDDIR)/src
	cp zlib-1.2.1/zconf.h $(MTDAAPDDIR)/src

gdbm:	
	if [ ! -f ./.configured ]; then \
	cd gdbm-1.8.3 && ./configure --prefix=$(TARGET)/lib --host=$(HOST) --enable-shared \
    --enable-static CC=$(CC); \
	fi
	make -C gdbm-1.8.3

id3tag:
	if [ ! -f ./.configured ]; then \
	cd libid3tag-0.15.0b && /bin/sh configure --prefix=$(TARGET) --host=$(HOST) CC=$(CC) AS=$(AS) AR=$(AR) \
	LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) CPPFLAGS=-I../zlib-1.2.1/ LDFLAGS=-L../zlib-1.2.1/ --disable-static;\
	cp -f ./patch/frametype.c ./; \
	cp -f ./patch/compat.c ./; \
	fi
	make -C libid3tag-0.15.0b

mtdaapd:
	if [ ! -f ./.configured ]; then \
	cd $(MTDAAPDDIR) && chmod 555 ./configure && /bin/sh ./configure --prefix=$(TARGET) --host=$(HOST) CC=$(CC) \
	CPPFLAGS="-I$(pwd)/libid3tag-0.15.0b -I$(pwd)/zlib-1.2.1 -I$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/include -I$(pwd)/gdbm-1.8.3 -I$(SQLITE3_INCS)" \
	LDFLAGS="-L$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/lib -L$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/lib/.libs -L$(pwd)/gdbm-1.8.3/.libs -L$(pwd)/libid3tag-0.15.0b -L$(pwd)/libid3tag-0.15.0b/.libs -L$(pwd)/zlib-1.2.1 -L$(SQLITE3_LIBS)" \
	LIBS=$(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/lib/.libs/*.o --with-id3tag=$(pwd)/libid3tag-0.15.0b \
    $(CONFIG_SQLITE) --disable-mdns; \
	fi
	chmod 444 $(MTDAAPDDIR)/config.status;
	chmod 444 $(MTDAAPDDIR)/configure;
	sleep 1
	make -C $(MTDAAPDDIR)

mkend:
	touch ./.configured;

install:
	if test ! -s "$(TARGET)/lib/libid3tag.so.0"; \
	then make -C libid3tag-0.15.0b/ install ;\
	rm -rf $(TARGET)/lib/include; \
	fi
	
	cp -f gdbm-1.8.3/.libs/libgdbm.so.3.0.0 $(TARGET)/lib/libgdbm.so.3
	#cp -rf $(MTDAAPDDIR)/admin-root/ $(TARGET)/etc/admin-root
	mkdir -p $(TARGET)/etc/admin-root
	touch $(TARGET)/etc/admin-root/index.html
	cp $(MTDAAPDDIR)/contrib/mt-daapd.conf $(TARGET)/var/.
ifeq ($(MTDAAPDDIR),mt-daapd-svn-1696)
	$(STRIP) $(MTDAAPDDIR)/src/.libs/mt-daapd
	$(STRIP) --strip-unneeded $(MTDAAPDDIR)/src/plugins/.libs/out-daap.so
	cp $(MTDAAPDDIR)/src/.libs/mt-daapd $(TARGET)/sbin/.
	cp $(MTDAAPDDIR)/src/plugins/.libs/out-daap.so $(TARGET)/lib/
else
	$(STRIP) $(MTDAAPDDIR)/src/mt-daapd
	cp $(MTDAAPDDIR)/src/mt-daapd $(TARGET)/sbin/.
endif
	ln -f -s /var/mt-daapd.conf $(TARGET)/etc/mt-daapd.conf

clean:
	make -C zlib-1.2.1/ clean
	make -C libid3tag-0.15.0b/ clean
	make -C gdbm-1.8.3 clean
	make -C $(MTDAAPDDIR)/ clean
	rm .configured

clean_config:
	rm .configured

.PHONY: all install clean

