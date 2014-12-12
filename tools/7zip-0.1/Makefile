# Tools:
RM=rm
CC=gcc
CXX=c++
CFLAGS=-g -Wall
#INSTALLPATH = /opt/projects/ntsw-sw/bcm47xx/brcm/hndtools-mipsel-linux-3.0/bin/

all: 7zip

7zip: LZMAEncoder.o 7zip.o
	@$(CXX) $(CFLAGS) -o 7zip LZMAEncoder.o 7zip.o
	cp -f 7zip ../

LZMAEncoder.o: LZMA/LZMAEncoder.cpp
	@$(CC) $(CFLAGS) -c LZMA/LZMAEncoder.cpp

7zip.o: 7zip.c
	@$(CC) $(CFLAGS) -c 7zip.c

clean:
	@$(RM) -f *.o
	@$(RM) -f 7zip

make fresh:
	make clean
	make all
