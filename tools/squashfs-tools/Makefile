INCLUDEDIR = .
LZMAPATH = ./lzma/SRC/7zip/Compress/LZMA_Lib

CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O2

all: mksquashfs mksquashfs-lzma lzma_alone mksquashfs_mknod mksquashfs_mknod-lzma

mksquashfs_mknod: mksquashfs_mknod.o read_fs.o sort.o
	$(CC) mksquashfs_mknod.o read_fs.o sort.o -lz -o $@

mksquashfs_mknod-lzma: mksquashfs_mknod.o read_fs.o sort.o
	make -C ./lzma/SRC/7zip/Compress/LZMA_Lib
	$(CXX) mksquashfs_mknod.o read_fs.o sort.o -L$(LZMAPATH) -llzma -o $@

mksquashfs: mksquashfs.o read_fs.o sort.o
	$(CC) mksquashfs.o read_fs.o sort.o -lz -o $@

mksquashfs-lzma: mksquashfs.o read_fs.o sort.o
	make -C ./lzma/SRC/7zip/Compress/LZMA_Lib
	$(CXX) mksquashfs.o read_fs.o sort.o -L$(LZMAPATH) -llzma -o $@
lzma_alone:
	make -C ./lzma/SRC/7zip/Compress/LZMA_Alone
	cp -f ./lzma/SRC/7zip/Compress/LZMA_Alone/lzma ./lzma

mksquashfs.o: mksquashfs.c mksquashfs.h

read_fs.o: read_fs.c read_fs.h

sort.o: sort.c


clean:
	@rm -f *.o mksquashfs mksquashfs-lzma lzma/lzma
	@make -C ./lzma/SRC/7zip/Compress/LZMA_Lib clean
	@make -C ./lzma/SRC/7zip/Compress/LZMA_Alone clean

.PHONY: mksquashfs mksquashfs-lzma all
