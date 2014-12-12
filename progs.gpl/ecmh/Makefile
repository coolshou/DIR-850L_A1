-include ../arch.mk
-include ../../path.mk
-include ../../.config

APPLET		= ecmh
ECMH_PATH	= ecmh-2005.02.09
ECMH_TB		= ecmh_2005.02.09.tar.bz2
PREFIX		= /usr/sbin
VPATH		= $(ECMH_PATH)/src

ALPHA_BUILD_DATE	=	$(shell date +%F)
ALPHA_BUILD_DATE	+=	$(shell date +%T)
ALPHA_VERSION		=	2009.09.18

ECMH			= ecmh
ECMH_VERSION	= 2005.02.09
#ECMH_OPTIONS	= -DECMH_SUPPORT_MLD2 -DECMH_GETIFADDR
ECMH_OPTIONS	= -DECMH_SUPPORT_MLD2

# DEBUG mode
#ECMH_OPTIONS	+= -DDEBUG

ifeq ($(shell echo $(ECMH_OPTIONS) | grep -c "DEBUG"),1)
ECMH_VERSION:=$(ECMH_VERSION)-debug
CFLAGS += -g
endif

SRCS    = ecmh.c linklist.c common.c interfaces.c groups.c grpint.c subscr.c
INCS    = ecmh.h linklist.h common.h interfaces.h groups.h grpint.h subscr.h mld.h
OBJS    = ecmh.o linklist.o common.o interfaces.o groups.o grpint.o subscr.o
CFLAGS += -D_GNU_SOURCE -D'ECMH_VERSION="$(ECMH_VERSION)"' -D'ALPHA_VERSION="$(ALPHA_VERSION)"' -D'ALPHA_BUILD_DATE="$(ALPHA_BUILD_DATE)"' $(ECMH_OPTIONS)
###############################################################################

all: ecmh

$(APPLET): $(OBJS) $(INCS)
	@echo -e "\033[32mBuilding $(APPLET) ...\033[0m"
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

install: all
	@echo -e "\033[32mInstalling $(APPLET) ...\033[0m"
	$(Q)$(STRIP) ./ecmh
	$(Q)[ -d $(TARGET)$(PREFIX) ] || mkdir -p $(TARGET)$(PREFIX)
	$(Q)cp -f ./ecmh $(TARGET)$(PREFIX)
	$(Q)chmod +x $(TARGET)$(PREFIX)/ecmh

mk_patch:
	$(Q)if [ ! -d ./$(ECMH_PATH) ]; then \
        echo -e "\033[32mNo ecmh source existed: Please make sure what you want to do ?\033[0m"; \
    else \
        echo -e "\033[32mRename $(ECMH_PATH) as $(ECMH_PATH).NEW...\033[0m"; \
        mv $(ECMH_PATH) $(ECMH_PATH).NEW; \
		tar jxf ./$(ECMH_TB); \
        diff -Naur -x .svn $(ECMH_PATH) $(ECMH_PATH).NEW > new.patch; \
		rm -fr $(ECMH_PATH); mv $(ECMH_PATH).NEW $(ECMH_PATH); \
        echo -e "\033[32mmk_patch is done:\033[0m"; \
        echo -e "\033[32m  check \"new.patch\".\033[0m"; \
    fi

clean:
	@echo -e "\033[32mCleaning *.o and ecmh ...\033[0m"
	$(Q)rm -f *.o ecmh

distclean: clean

.PHONY: all ecmh install mk_patch clean distclean

