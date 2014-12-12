INCLUDEDIR = .
LZMAPATH = ./lzma/C/7zip/Compress/LZMA_Lib

CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O2

all: mksquashfs mksquashfs-lzma lzma_alone

mksquashfs: mksquashfs.o read_fs.o sort.o
	$(CC) mksquashfs.o read_fs.o sort.o -lz -o $@

mksquashfs-lzma: mksquashfs.o read_fs.o sort.o
	make -C $(LZMAPATH)
	$(CXX) mksquashfs.o read_fs.o sort.o -L$(LZMAPATH) -llzma -o $@
lzma_alone:
	make -C ./lzma/C/7zip/Compress/LZMA_Alone
	cp -f ./lzma/C/7zip/Compress/LZMA_Alone/lzma ./lzma

mksquashfs.o: mksquashfs.c squashfs_fs.h mksquashfs.h global.h sort.h

read_fs.o: read_fs.c squashfs_fs.h read_fs.h global.h

sort.o: sort.c squashfs_fs.h global.h sort.h

unsquashfs: unsquashfs.o
	make -C $(LZMAPATH)
	$(CXX) unsquashfs.o -L$(LZMAPATH) -llzma -o $@

unsquashfs.o: unsquashfs.c squashfs_fs.h read_fs.h global.h

clean:
	@rm -f *.o mksquashfs mksquashfs-lzma lzma/lzma
	@make -C ./lzma/C/7zip/Compress/LZMA_Lib clean
	@make -C ./lzma/C/7zip/Compress/LZMA_Alone clean

.PHONY: mksquashfs mksquashfs-lzma all
