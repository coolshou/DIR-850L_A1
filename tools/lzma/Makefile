# makefile for lzma

LZMA	:= lzma-4.32.6
TARBALL	:= $(LZMA).tar.gz
UNTAR	:= tar zxf

all: lzma

clean:
	@rm -rf $(LZMA) lzma

lzma-4.32.6/Makefile:
	@$(UNTAR) $(TARBALL)
	@cd $(LZMA); ./configure

lzma: lzma-4.32.6/Makefile
	@cd $(LZMA); make
	@cp lzma-4.32.6/src/lzma/lzma .



.PHONY: clean
