-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config

pwd = $(shell pwd)

all: iconv mkend  

iconv:	
	if [ ! -f ./.configured ]; then \
	cd libiconv-1.9.1 && /bin/sh configure --prefix=$(TARGET) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) --enable-static;\
	fi	
	cp libiconv-1.9.1/srclib/Makefile_1 libiconv-1.9.1/srclib/Makefile
	make -C libiconv-1.9.1


mkend:
	touch ./.configured;

install:
	if test ! -s "$(TARGET)/lib/libiconv.so.2.2.0"; \
	then cp libiconv-1.9.1/lib/.libs/libiconv.so.2.2.0 $(TARGET)/lib/. ;\
	ln -s -f libiconv.so.2.2.0 $(TARGET)/lib/libiconv.so ;\
	ln -s -f libiconv.so.2.2.0 $(TARGET)/lib/libiconv.so.2 ;\
	fi
	
clean:
	make -C libiconv-1.9.1 clean
	rm -f .configured

.PHONY: all install clean
