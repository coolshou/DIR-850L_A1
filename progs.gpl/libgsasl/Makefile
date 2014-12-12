-include ../arch.mk
-include ../../path.mk

GSASLPATH=libgsasl-0.2.6
LIBS=$(shell echo -L$(CC_PATH)/lib/)

all:
	@if [ ! -f ./$(GSASLPATH)/Makefile ]; then \
		echo -e "\033[32mlibgsasl hasn't be config before, configuring...\033[0m"; \
		export LIBS=$(LIBS); \
		cd ./$(GSASLPATH); \
		./configure --build=i386-pc-linux-gnu --host=$(HOST_TYPE) --target=$(HOST_TYPE) --disable-gssapi; \
	fi
	echo -e "\033[32mCompiling libgsasl ...\033[0m"
	make -C $(GSASLPATH)

install:

clean:
	@if [ -f ./$(GSASLPATH)/Makefile ]; then \
	make -C $(GSASLPATH) distclean; \
	rm -f ./$(GSASLPATH)/Makefile; \
	fi

.PHONY: all install clean
