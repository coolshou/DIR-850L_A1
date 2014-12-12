#
# Makefile for the zip program
# for the Linux OS
#

-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config

pwd = $(shell pwd)
APPLET := 
LOCAL_SRCS := 
LIBJPEG_DIR := jpeg-8c
LIBPNG_DIR := libpng-1.5.4
LIBTIFF_DIR := tiff-3.9.5
NETPBM_DIR:= netpbm-10.35.81
ZLIB_PATH := $(pwd)/../zlib-1.2.3
LIBJPEG_PATH := $(pwd)/$(LIBJPEG_DIR)
LIBPNG_PATH := $(pwd)/$(LIBPNG_DIR)
LIBTIFF_PATH := $(pwd)/$(LIBTIFF_DIR)
#CFLAGS += 
#LDFLAGS += 

ifeq ($(ELBOX_MINIDLNA),y)
PREBUILD := 
else
PREBUILD := netpbm png2pnm
endif

#
# Primary link
#
-include $(TOPDIR)/Rules.mk
#CMLIB_OBJS += $(LOCAL_LIBS)

libjpeg:
	@if [ ! -f "$(LIBJPEG_DIR)/Makefile" ]; then \
	   cd $(LIBJPEG_DIR) ; \
	   ./configure --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	   LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) --disable-static --with-gnu-ld ;\
	   cd ..; \
	fi
	make -C $(LIBJPEG_DIR)

libpng: zlib
	@if [ ! -f "$(LIBPNG_DIR)/Makefile" ]; then \
	   cd $(LIBPNG_DIR); \
	   ./configure --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	   LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) LDFLAGS="-L$(ZLIB_PATH)" \
	   CFLAGS="-I$(ZLIB_PATH)" --disable-static --with-gnu-ld ;\
	   cd ..; \
	fi
	make -C $(LIBPNG_DIR)
	ln -s -f libpng15.so $(LIBPNG_DIR)/.libs/libpng.so

png2pnm: libpng
	$(CC) $(CFLAGS) -I$(pwd)/$(LIBPNG_DIR) -I$(ZLIB_PATH) $(LDFLAGS) -L$(ZLIB_PATH) -L$(pwd)/$(LIBPNG_DIR)/.libs/ -lpng -lz -lm -o $(LIBPNG_DIR)/contrib/pngminus/png2pnm $(LIBPNG_DIR)/contrib/pngminus/png2pnm.c

zlib: $(ZLIB_PATH)/libz.so
	make -C $(ZLIB_PATH)

libtiff: libjpeg zlib
	@if [ ! -f "$(LIBTIFF_DIR)/Makefile" ]; then \
	   cd $(LIBTIFF_DIR) ; \
	   ./configure --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	   LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) \
	   LDFLAGS="-L$(ZLIB_PATH) -L$(LIBJPEG_PATH)/.libs" \
	   CFLAGS="-I$(ZLIB_PATH) -I$(LIBJPEG_PATH)" --disable-static --with-gnu-ld --disable-jbig \
	   --disable-cxx ;\
	   cd ..; \
	fi
	make -C $(LIBTIFF_DIR)

netpbm: libpng libjpeg libtiff zlib
	@if [ ! -f "$(NETPBM_DIR)/Makefile.config" ]; then \
		CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) TIFFHDR_DIR=$(LIBTIFF_PATH)/libtiff/ \
		TIFFLIB=$(LIBTIFF_PATH)/libtiff/.libs/libtiff.so JPEGHDR_DIR=$(LIBJPEG_PATH) \
		JPEGLIB=$(LIBJPEG_PATH)/.libs/libjpeg.so PNGHDR_DIR=$(LIBPNG_PATH) \
		PNGLIB=$(LIBPNG_PATH)/.libs/libpng.so ZHDR_DIR=$(ZLIB_PATH) ZLIB=$(ZLIB_PATH)/libz.so \
		./gen_netpbm_config.sh; \
	fi
	make -C $(NETPBM_DIR)/lib
	make -C $(NETPBM_DIR)/converter/other tifftopnm
	make -C $(NETPBM_DIR)/converter/ppm winicontoppm
	make -C $(NETPBM_DIR)/editor/ pamscale

clean:
	@if [ -f "$(LIBJPEG_DIR)/Makefile" ]; then \
		make clean -C $(LIBJPEG_DIR); \
	fi
	@if [ -f "$(LIBPNG_DIR)/Makefile" ]; then \
		make clean -C $(LIBPNG_DIR); \
		rm -f $(LIBPNG_DIR)/contrib/pngminus/png2pnm; \
	fi
	@if [ -f "$(LIBTIFF_DIR)/Makefile" ]; then \
		make clean -C $(LIBTIFF_DIR); \
	fi
	@if [ -f "$(NETPBM_DIR)/Makefile.config" ]; then \
		make clean -C $(NETPBM_DIR); \
	fi
#@echo -e "\033[32mCleaning $(APPLET) ...\033[0m"
#	$(Q)find -name "*.o" | xargs rm -Rf
#	$(Q)rm -f $(APPLET) $(DEPEND_FILES) *~ *.o .depend $(APPLET)

distclean:
	@if [ -f "$(LIBJPEG_DIR)/Makefile" ]; then \
		make distclean -C $(LIBJPEG_DIR); \
	fi
	@if [ -f "$(LIBPNG_DIR)/Makefile" ]; then \
		make distclean -C $(LIBPNG_DIR); \
		rm $(LIBPNG_DIR)/contrib/pngminus/png2pnm; \
	fi
	@if [ -f "$(LIBTIFF_DIR)/Makefile" ]; then \
		make distclean -C $(LIBTIFF_DIR); \
	fi
	@if [ -f "$(NETPBM_DIR)/Makefile.config" ]; then \
		make distclean -C $(NETPBM_DIR); \
	fi

install: 
ifeq ($(ELBOX_MINIDLNA),y)
else
	@echo -e "\033[32mInstalling libjpeg ...\033[0m"
	@if [ -f "$(LIBJPEG_DIR)/.libs/libjpeg.so.8.3.0" ]; then \
		cp $(LIBJPEG_DIR)/.libs/libjpeg.so.8.3.0 $(TARGET)/lib/ ;\
		cp $(LIBJPEG_DIR)/.libs/cjpeg $(TARGET)/usr/bin/ ;\
		cp $(LIBJPEG_DIR)/.libs/djpeg $(TARGET)/usr/bin/ ;\
		cp $(LIBJPEG_DIR)/.libs/jpegtran $(TARGET)/usr/bin/ ;\
		ln -s -f libjpeg.so.8.3.0 $(TARGET)/lib/libjpeg.so ;\
		ln -s -f libjpeg.so.8.3.0 $(TARGET)/lib/libjpeg.so.8 ;\
		$(STRIP) $(TARGET)/lib/libjpeg.so.8.3.0 ; \
		$(STRIP) $(TARGET)/usr/bin/cjpeg ; \
		$(STRIP) $(TARGET)/usr/bin/djpeg ; \
		$(STRIP) $(TARGET)/usr/bin/jpegtran ; \
	fi

	@echo -e "\033[32mInstalling libpng ...\033[0m"
	@if [ -f "$(LIBPNG_DIR)/.libs/libpng15.so.15.4.0" ]; then \
		cp $(LIBPNG_DIR)/.libs/libpng15.so.15.4.0 $(TARGET)/lib/ ;\
		ln -s -f /lib/libpng15.so.15.4.0 $(TARGET)/lib/libpng.so ;\
		ln -s -f /lib/libpng15.so.15.4.0 $(TARGET)/lib/libpng15.so.15 ;\
		cp $(LIBPNG_DIR)/contrib/pngminus/png2pnm $(TARGET)/usr/bin/ ;\
		$(STRIP) $(TARGET)/lib/libpng15.so.15.4.0 ; \
		$(STRIP) $(TARGET)/usr/bin/png2pnm ; \
	fi

	@echo -e "\033[32mInstalling libtiff ...\033[0m"
	@if [ -f "$(LIBTIFF_DIR)/libtiff/.libs/libtiff.so.3.9.5" ]; then \
		cp $(LIBTIFF_DIR)/libtiff/.libs/libtiff.so.3.9.5 $(TARGET)/lib/ ;\
		ln -s -f /lib/libtiff.so.3.9.5 $(TARGET)/lib/libtiff.so.3 ;\
		cp $(LIBTIFF_DIR)/tools/.libs/tiff2rgba $(TARGET)/usr/bin/ ;\
		$(STRIP) $(TARGET)/lib/libtiff.so.3.9.5 ; \
		$(STRIP) $(TARGET)/usr/bin/tiff2rgba ; \
	fi

	@echo -e "\033[32mInstalling netpbm ...\033[0m"
	@if [ -f "$(NETPBM_DIR)/lib/libnetpbm.so.10.35" ]; then \
		cp $(NETPBM_DIR)/lib/libnetpbm.so.10.35 $(TARGET)/lib/ ;\
		ln -s -f /lib/libnetpbm.so.10.35 $(TARGET)/lib/libnetpbm.so.10 ;\
		cp $(NETPBM_DIR)/converter/other/tifftopnm $(TARGET)/usr/bin/ ;\
		cp $(NETPBM_DIR)/editor/pamscale $(TARGET)/usr/bin/ ;\
		$(STRIP) $(TARGET)/lib/libnetpbm.so.10.35 ; \
		$(STRIP) $(TARGET)/usr/bin/tifftopnm ; \
		$(STRIP) $(TARGET)/usr/bin/pamscale ; \
	fi
endif

#	@echo -e "\033[32mInstalling $(APPLET) ...\033[0m"
#	cp $(APPLET) $(TARGET)/usr/sbin
#	$(STRIP) $(TARGET)/usr/sbin/$(APPLET)

.PHONY: install clean all
