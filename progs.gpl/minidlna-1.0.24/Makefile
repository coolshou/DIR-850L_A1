-include ../arch.mk
-include ../../path.mk
-include $(TOPDIR)/.config
-include lib.info

pwd = $(shell pwd)
tmp_inc = rootfs/include
tmp_lib = rootfs/lib

all: libiconv sqlite libjpeg id3tag libexif libogg libvorbis libflac ffmpeg e2fs install_temp_file minidlna

libiconv:
	rm -f $(TARGET)/lib/libiconv.so*
	make -C $(PATH_LIB_ICONV)
    
sqlite:
	make -C $(PATH_LIB_SQLITE)

libjpeg:
	rm -f $(TARGET)/lib/libjpeg.so*
	make -C $(TOPDIR)/progs.gpl/image_tools/ libjpeg

id3tag:
	make -C $(PATH_LIB_ID3TAG) id3tag
	$(STRIP) $(INC_LIB_ID3TAG)/.libs/libid3tag.so.0.2.0

#	make -C $(TOPDIR)/progs.gpl/itunes_server/ zlib
#	cd $(LIB_ID3TAG) && /bin/sh configure --prefix=$(TARGET) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
#	LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) CPPFLAGS=-I$(TOPDIR)/progs.gpl/itunes_server/zlib-1.2.1/ \
#	LDFLAGS=-L$(TOPDIR)/progs.gpl/itunes_server/zlib-1.2.1/ --disable-static; \
#	cd $(pwd)
#	make -C $(LIB_ID3TAG)


libexif:
	if [ ! -f ./.configured.libexif ]; then \
	cd $(LIB_EXIF) && /bin/sh configure --prefix=$(TARGET) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) --enable-shared; \
	cd $(pwd); \
	fi
	make -C $(LIB_EXIF)
	$(STRIP) $(LIB_EXIF)/libexif/.libs/libexif.so.12.3.2
	touch ./.configured.libexif

libogg:
	if [ ! -f ./.configured.libogg ]; then \
	cd $(LIB_OGG)&& /bin/sh configure --prefix=$(TARGET) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) --enable-shared; \
	cd $(pwd); \
	fi
	make -C $(LIB_OGG)
	$(STRIP) $(LIB_OGG)/src/.libs/libogg.so.0.8.0
	touch ./.configured.libogg

libvorbis:
	if [ ! -f ./.configured.libvorbis ]; then \
	cd $(LIB_VORBIS)&& /bin/sh configure --prefix=$(TARGET) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	LD=$(LD) RANLIB=$(RANLIB) STRIP=$(STRIP) --with-ogg-includes=$(pwd)/$(LIB_OGG)/include \
	--with-ogg=$(pwd)/$(LIB_OGG) --with-ogg-libraries=$(pwd)/$(LIB_OGG)/src/.libs --disable-oggtest --enable-shared; \
	cd $(pwd); \
	fi
	make -C $(LIB_VORBIS)
	$(STRIP) $(LIB_VORBIS)/lib/.libs/libvorbis.so.0.4.5
	touch ./.configured.libvorbis
	
libflac:
	if [ ! -f ./.configured.libflac ]; then \
	cd $(LIB_FLAC) && /bin/sh configure --prefix=$(TARGET) --host=$(HOST_TYPE) CC=$(CC) AS=$(AS) AR=$(AR) \
	LD=$(LD) CXX=$(CXX) RANLIB=$(RANLIB) STRIP=$(STRIP) --with-ogg-includes=$(pwd)/$(LIB_OGG)/include \
	--with-ogg=$(pwd)/$(LIB_OGG) --with-ogg-libraries=$(pwd)/$(LIB_OGG)/src/.libs --disable-oggtest --disable-thorough-tests \
	--disable-cpplibs --enable-shared; \
	cd $(pwd); \
	fi
	make -C $(LIB_FLAC)
	$(STRIP) $(LIB_FLAC)/src/libFLAC/.libs/libFLAC.so.8.2.0
	touch ./.configured.libflac

ffmpeg:	
	if [ ! -f ./.configured.ffmpeg ]; then \
	cd $(LIB_FFMPEG) && /bin/sh configure --prefix=$(TARGET) --cross-prefix=$(CROSS_COMPILE) --arch=$(HOST_CPU) --target-os=linux \
	--enable-shared --enable-gpl --enable-pthreads --enable-small --enable-postproc --disable-debug --disable-devices \
	--disable-bsfs --disable-stripping --disable-ffprobe --disable-parsers --disable-encoders --disable-decoders --disable-muxers \
	--disable-mmx --disable-mmx2; \
	cd $(pwd); \
	fi
	make -C $(LIB_FFMPEG)
	$(STRIP) $(INC_LIB_AVFORMAT)/libavformat.so.53
	$(STRIP) $(INC_LIB_AVCODEC)/libavcodec.so.53
	$(STRIP) $(INC_LIB_AVUTIL)/libavutil.so.51
	touch ./.configured.ffmpeg

e2fs:
	if [ ! -f ./.configured.libe2fs ]; then \
	cd $(LIB_E2FS) && /bin/sh configure --prefix=$(TARGET) --host=$(HOST_TYPE) CC=$(CC) \
	LD=$(LD) --enable-elf-shlibs --enable-libuuid; \
	cd $(pwd); \
	fi
	make -C $(LIB_E2FS)
	$(STRIP) $(LIB_E2FS)/lib/libuuid.so.1.2
	touch ./.configured.libe2fs
	
minidlna:
	cp config_alpha.h $(APP_MINIDLNA)/config.h
	make -C $(APP_MINIDLNA)
	$(STRIP) $(APP_MINIDLNA)/minidlna

install_lib:
	make -C $(PATH_LIB_ICONV) install
	make -C $(PATH_LIB_SQLITE) install
	cp $(INC_LIB_JPEG)/.libs/libjpeg.so.8.3.0 $(TARGET)/lib/ ;\
	ln -s -f libjpeg.so.8.3.0 $(TARGET)/lib/libjpeg.so ;\
	ln -s -f libjpeg.so.8.3.0 $(TARGET)/lib/libjpeg.so.8 ;
	
	cp $(INC_LIB_ID3TAG)/.libs/libid3tag.so.0.2.0 $(TARGET)/lib/ ;\
	ln -s -f libid3tag.so.0.2.0 $(TARGET)/lib/libid3tag.so ;\
	ln -s -f libid3tag.so.0.2.0 $(TARGET)/lib/libid3tag.so.0 ;
	
	cp $(LIB_EXIF)/libexif/.libs/libexif.so.12.3.2 $(TARGET)/lib/ ;\
	ln -s -f libexif.so.12.3.2 $(TARGET)/lib/libexif.so ;\
	ln -s -f libexif.so.12.3.2 $(TARGET)/lib/libexif.so.12 ;
	
	cp $(LIB_OGG)/src/.libs/libogg.so.0.8.0 $(TARGET)/lib/ ;\
	ln -s -f libogg.so.0.8.0 $(TARGET)/lib/libogg.so ;\
	ln -s -f libogg.so.0.8.0 $(TARGET)/lib/libogg.so.0 ;
	
	cp $(LIB_VORBIS)/lib/.libs/libvorbis.so.0.4.5 $(TARGET)/lib/ ;\
	ln -s -f libvorbis.so.0.4.5 $(TARGET)/lib/libvorbis.so; \
	ln -s -f libvorbis.so.0.4.5 $(TARGET)/lib/libvorbis.so.0; 
	
	cp $(LIB_FLAC)/src/libFLAC/.libs/libFLAC.so.8.2.0 $(TARGET)/lib/ ;\
	ln -s -f libFLAC.so.8.2.0 $(TARGET)/lib/libFLAC.so ;\
	ln -s -f libFLAC.so.8.2.0 $(TARGET)/lib/libFLAC.so.8 ;
	
	cp $(INC_LIB_AVUTIL)/libavutil.so.51 $(TARGET)/lib/ ;\
	ln -s -f libavutil.so.51 $(TARGET)/lib/libavutil.so ;
	
	cp $(INC_LIB_AVFORMAT)/libavformat.so.53 $(TARGET)/lib/ ;\
	ln -s -f libavformat.so.53 $(TARGET)/lib/libavformat.so ;
	
	cp $(INC_LIB_AVCODEC)/libavcodec.so.53 $(TARGET)/lib/ ;\
	ln -s -f libavcodec.so.53 $(TARGET)/lib/libavcodec.so ;
	
	cp $(INC_LIB_AVCODEC)/libavcodec.so.53 $(TARGET)/lib/ ;\
	ln -s -f libavcodec.so.53 $(TARGET)/lib/libavcodec.so ;
	
	cp $(LIB_E2FS)/lib/libuuid.so.1.2 $(TARGET)/lib/ ;\
	ln -s -f libuuid.so.1.2 $(TARGET)/lib/libuuid.so ;\
	ln -s -f libuuid.so.1.2 $(TARGET)/lib/libuuid.so.1 ;
	
install_temp_file:
	mkdir -p $(tmp_inc)
	mkdir -p $(tmp_lib)	
	mkdir -p $(tmp_inc)/libexif
	mkdir -p $(tmp_inc)/ogg
	mkdir -p $(tmp_inc)/vorbis
	mkdir -p $(tmp_inc)/FLAC
	mkdir -p $(tmp_inc)/ffmpeg
	mkdir -p $(tmp_inc)/ffmpeg/libavutil
	mkdir -p $(tmp_inc)/ffmpeg/libavformat
	mkdir -p $(tmp_inc)/ffmpeg/libavcodec
	cp $(TOPDIR)/progs.gpl/libiconv/libiconv-1.9.1/include/iconv.h $(tmp_inc)
	cp $(INC_LIB_SQLITE)/sqlite3.h $(tmp_inc)
	cp $(INC_LIB_JPEG)/jpeglib.h $(tmp_inc)
	cp $(INC_LIB_ID3TAG)/id3tag.h $(tmp_inc)
	cp $(INC_LIB_EXIF)/libexif/exif-loader.h $(tmp_inc)/libexif
	cp $(INC_LIB_OGG)/ogg/ogg.h $(tmp_inc)/ogg
	cp $(INC_LIB_VORBIS)/vorbis/codec.h $(tmp_inc)/vorbis
	cp $(INC_LIB_FLAC)/FLAC/metadata.h $(tmp_inc)/FLAC
	cp $(INC_LIB_AVUTIL)/avutil.h $(tmp_inc)/ffmpeg/libavutil
	cp $(INC_LIB_AVFORMAT)/avformat.h $(tmp_inc)/ffmpeg/libavformat
	cp $(INC_LIB_AVCODEC)/avcodec.h $(tmp_inc)/ffmpeg/libavcodec
	cp $(PATH_LIB_SQLITE)/rootfs/lib/libsqlite3.so.0.8.6 $(tmp_lib) ;\
	cp $(PATH_LIB_ICONV)/$(LIB_ICONV)/lib/.libs/libiconv.so.2.2.0 $(tmp_lib) ;\
	cp $(INC_LIB_JPEG)/.libs/libjpeg.so.8.3.0 $(tmp_lib) ;\
	cp $(INC_LIB_ID3TAG)/.libs/libid3tag.so.0.2.0 $(tmp_lib) ;\
	cp $(LIB_EXIF)/libexif/.libs/libexif.so.12.3.2 $(tmp_lib) ;\
	cp $(LIB_OGG)/src/.libs/libogg.so.0.8.0 $(tmp_lib) ;\
	cp $(LIB_VORBIS)/lib/.libs/libvorbis.so.0.4.5 $(tmp_lib) ;\
	cp $(LIB_FLAC)/src/libFLAC/.libs/libFLAC.so.8.2.0 $(tmp_lib) ;\
	cp $(INC_LIB_AVUTIL)/libavutil.so.51 $(tmp_lib) ;\
	cp $(INC_LIB_AVFORMAT)/libavformat.so.53 $(tmp_lib) ;\
	cp $(INC_LIB_AVCODEC)/libavcodec.so.53 $(tmp_lib) ;\
	cp $(INC_LIB_AVCODEC)/libavcodec.so.53 $(tmp_lib) ;\
	cp $(LIB_E2FS)/lib/libuuid.so.1.2 $(tmp_lib) ;\
	cd $(tmp_lib) ;\
	ln -s -f libsqlite3.so.0.8.6 libsqlite3.so ;\
	ln -s -f libsqlite3.so.0.8.6 libsqlite3.so.0 ;\
	ln -s -f libiconv.so.2.2.0 libiconv.so ;\
	ln -s -f libiconv.so.2.2.0 libiconv.so.2 ;\
	ln -s -f libjpeg.so.8.3.0 libjpeg.so ;\
	ln -s -f libjpeg.so.8.3.0 libjpeg.so.8 ;\
	ln -s -f libid3tag.so.0.2.0 libid3tag.so ;\
	ln -s -f libid3tag.so.0.2.0 libid3tag.so.0 ;\
	ln -s -f libexif.so.12.3.2 libexif.so ;\
	ln -s -f libexif.so.12.3.2 libexif.so.12 ;\
	ln -s -f libogg.so.0.8.0 libogg.so ;\
	ln -s -f libogg.so.0.8.0 libogg.so.0 ;\
	ln -s -f libvorbis.so.0.4.5 libvorbis.so; \
	ln -s -f libvorbis.so.0.4.5 libvorbis.so.0; \
	ln -s -f libFLAC.so.8.2.0 libFLAC.so ;\
	ln -s -f libFLAC.so.8.2.0 libFLAC.so.8 ;\
	ln -s -f libavutil.so.51 libavutil.so ;\
	ln -s -f libavformat.so.53 libavformat.so ;\
	ln -s -f libavcodec.so.53 libavcodec.so ;\
	ln -s -f libavcodec.so.53 libavcodec.so ;\
	ln -s -f libuuid.so.1.2 libuuid.so ;\
	ln -s -f libuuid.so.1.2 libuuid.so.1 ;\
	cd $(pwd);
	
install: install_lib
	cp $(APP_MINIDLNA)/minidlna $(TARGET)/usr/bin
	#cp minidlna.conf $(TARGET)/etc
	
clean:
	make -C $(LIB_EXIF) clean
	make -C $(LIB_OGG) clean
	make -C $(LIB_VORBIS) clean
	make -C $(LIB_FLAC) clean
	make -C $(LIB_FFMPEG) clean
	make -C $(LIB_E2FS) clean
	make -C $(APP_MINIDLNA) clean
	rm -rf rootfs
	rm .configured.*

.PHONY: all install clean
