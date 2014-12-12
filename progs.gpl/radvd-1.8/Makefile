-include ../arch.mk
-include ../../path.mk
-include ../../.config

APPLET		= radvd
RADVD_PATH	= radvd-1.8
PREFIX		= /usr/sbin

CLEANFILES = \
	gram.c \
	gram.h \
	radvd.8 \
	radvd.conf.5 \
	radvdump.8 \
	scanner.c

###############################################################################
ifeq ($(strip $(ELBOX_PROGS_GPL_RADVD_NOUSER_ALPHA)),y)
CFLAGS += -DALPHA_NO_USER_SUPPORT
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_RADVD_NOLIBFL)),y)
CFLAGS += -DALPHA_NO_LIBFL_SUPPORT
endif

#CFLAGS += -DALPHA_IPV6_LOGO_TEST
###############################################################################

all: radvd
radvd: 
	$(Q)if [ ! -f ./$(RADVD_PATH)/Makefile ]; then \
		echo -e "\033[32mradvd hasn't be config before, configuring...\033[0m"; \
		cd ./$(RADVD_PATH); \
		./configure --build=i386-linux --host=$(HOST_TYPE) CFLAGS="$(CFLAGS)"; \
	fi
	@echo -e "\033[32mCompiling radvd ...\033[0m"
#	$(Q)make -C $(RADVD_PATH) radvd_LDADD=""
	$(Q)make -C $(RADVD_PATH) 

install:
	@echo -e "\033[32mInstalling radvd ...\033[0m"
	$(Q)$(STRIP) $(RADVD_PATH)/radvd
	$(Q)$(STRIP) $(RADVD_PATH)/radvdump
	$(Q)[ -d $(TARGET)$(PREFIX) ] || mkdir -p $(TARGET)$(PREFIX)
	$(Q)cp -f ./$(RADVD_PATH)/radvd ./$(RADVD_PATH)/radvdump $(TARGET)$(PREFIX)
	$(Q)chmod +x $(TARGET)$(PREFIX)/radvd
	$(Q)chmod +x $(TARGET)$(PREFIX)/radvdump

clean:
	@echo -e "\033[32mCleaning radvd ...\033[0m"
#	$(Q) rm -fr ./$(RADVD_PATH)
	$(Q) rm -f ./$(RADVD_PATH)/*.o ./$(RADVD_PATH)/radvd ./$(RADVD_PATH)/Makefile
	$(Q) cd ./$(RADVD_PATH); \
		test -z "$(CLEANFILES)" || rm -f $(CLEANFILES) 

.PHONY: all install clean

