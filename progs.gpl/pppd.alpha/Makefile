#
# pppd makefile for Linux
# $Id: Makefile,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $
#

-include ../arch.mk
-include ../../path.mk
-include ../../.config

TARGETS = pppd

PPPDSRCS = main.c magic.c fsm.c lcp.c ipcp.c upap.c chap.c md5.c ccp.c ecp.c \
	   ipxcp.c auth.c options.c sys-linux.c md4.c chap_ms.c \
	   demand.c utils.c tty.c eap.c eloop.c dtrace.c ppp_fcs.c\
	   pppoe/pppoe_options.c pppoe/pppoe.c pppoe/if.c pppoe/debug.c pppoe/common.c pppoe/discovery.c pppoe/ppp.c \
	   kpppoe/kpppoe_options.c kpppoe/kpppoe.c kpppoe/discovery.c 

HEADERS = ccp.h chap.h ecp.h fsm.h ipcp.h \
	  ipxcp.h lcp.h magic.h md5.h patchlevel.h pathnames.h pppd.h ppp_fcs.h\
	  upap.h eap.h eloop.h dtrace.h

PPPDOBJS = main.o magic.o fsm.o lcp.o ipcp.o upap.o chap.o md5.o ccp.o ecp.o \
	   auth.o options.o demand.o utils.o sys-linux.o ipxcp.o tty.o eap.o eloop.o dtrace.o ppp_fcs.o\
	   pppoe/pppoe_options.o pppoe/pppoe.o pppoe/if.o pppoe/debug.o pppoe/common.o pppoe/discovery.o pppoe/ppp.o \
	   kpppoe/kpppoe_options.o kpppoe/kpppoe.o kpppoe/discovery.o 

ifeq ($(strip $(ELBOX_PROGS_GPL_PPPD_ACCEL_PPTP)),y)
ifeq ($(strip $(ELBOX_PROGS_GPL_PPPD_ACCEL_PPTP_PLUS)),y)
PPPDSRCS += accel-pptp/pptp_0_7_13.c accel-pptp/pptp_callmgr.c accel-pptp/pptp_ctrl_0_7_13.c accel-pptp/util.c \
	   accel-pptp/vector.c accel-pptp/dirutil.c accel-pptp/pptp_quirks.c accel-pptp/orckit_quirks.c
PPPDOBJS += accel-pptp/pptp_0_7_13.o accel-pptp/pptp_callmgr.o accel-pptp/pptp_ctrl_0_7_13.o accel-pptp/util.o \
	   accel-pptp/vector.o accel-pptp/dirutil.o accel-pptp/pptp_quirks.o accel-pptp/orckit_quirks.o 
else
PPPDSRCS += accel-pptp/pptp_ctrl.c accel-pptp/pptp.c 
PPPDOBJS += accel-pptp/pptp_ctrl.o accel-pptp/pptp.o 
endif
CFLAGS += -DCONFIG_PPTP
else
PPPDSRCS += pptp/pptp_options.c pptp/pqueue.c pptp/pptp_ctrl.c pptp/pptp_gre.c 
PPPDOBJS += pptp/pptp_options.o pptp/pqueue.o pptp/pptp_ctrl.o pptp/pptp_gre.o 
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_PPPD_IPV6_L2TP)),y)
PPPDSRCS += l2tp_ipv6/debug.c l2tp_ipv6/l2tp_options.c l2tp_ipv6/l2tp.c l2tp_ipv6/dgram.c l2tp_ipv6/utils.c l2tp_ipv6/peer.c l2tp_ipv6/session.c l2tp_ipv6/tunnel.c l2tp_ipv6/sync-pppd.c l2tp_ipv6/async-pppd.c 
PPPDOBJS += l2tp_ipv6/debug.o l2tp_ipv6/l2tp_options.o l2tp_ipv6/l2tp.o l2tp_ipv6/dgram.o l2tp_ipv6/utils.o l2tp_ipv6/peer.o l2tp_ipv6/session.o l2tp_ipv6/tunnel.o l2tp_ipv6/sync-pppd.o l2tp_ipv6/async-pppd.o
else
PPPDSRCS += l2tp/debug.c l2tp/l2tp_options.c l2tp/l2tp.c l2tp/dgram.c l2tp/utils.c l2tp/peer.c l2tp/session.c l2tp/tunnel.c l2tp/sync-pppd.c l2tp/async-pppd.c 
PPPDOBJS += l2tp/debug.o l2tp/l2tp_options.o l2tp/l2tp.o l2tp/dgram.o l2tp/utils.o l2tp/peer.o l2tp/session.o l2tp/tunnel.o l2tp/sync-pppd.o l2tp/async-pppd.o
endif

#PPP_CHAT=y
#ifdef PPP_CHAT
ifeq ($(strip $(ELBOX_PROGS_GPL_PPPD_ALPHA)),y)
PPPDSRCS += ppp3g/ppp3g.c ppp3g/ppp3g_options.c ppp3g/chat.c
PPPDOBJS += ppp3g/ppp3g.o ppp3g/ppp3g_options.o ppp3g/chat.o
CFLAGS += -DHAVE_PPP3G_CHAT=1
endif

#
LIBS =
COPTS = -O2 -pipe -Wall -g -D_linux_=1
#COPTS+= -DDDEBUG
#COPTS+= -DDEBUG_ELOOP

UCLINUX=y
# Uncomment the next 2 lines to include support for Microsoft's
# MS-CHAP authentication protocol.  Also, edit plugins/radius/Makefile.linux.
CHAPMS=y
USE_CRYPT=y
# Don't use MSLANMAN unless you really know what you're doing.
#MSLANMAN=y
# Uncomment the next line to include support for MPPE.  CHAPMS (above) must
# also be enabled.  Also, edit plugins/radius/Makefile.linux.
MPPE=y

# Uncomment the next line to include support for PPP packet filtering.
# This requires that the libpcap library and headers be installed
# and that the kernel driver support PPP packet filtering, which it
# doesn't yet.
#FILTER=y

# Uncomment the next line to enable multilink PPP (enabled by default)
# Linux distributions: Please leave multilink ENABLED in your builds
# of pppd!
#HAVE_MULTILINK=y

# Uncomment the next line to enable the TDB database (enabled by default.)
# If you enable multilink, then TDB is automatically enabled also.
# Linux distributions: Please leave TDB ENABLED in your builds.
#USE_TDB=y

#HAS_SHADOW=y
#USE_PAM=y
#HAVE_INET6=y

# Enable plugins
#PLUGIN=y

# Enable Microsoft proprietary Callback Control Protocol
#CBCP=y

# Enable EAP SRP-SHA1 authentication (requires libsrp)
#USE_SRP=y

MAXOCTETS=y

INCLUDE_DIRS= -I. -I./include -I../../include -I./include/linux

COMPILE_FLAGS= -DHAVE_PATHS_H #-DIPX_CHANGE -DHAVE_MMAP

CFLAGS+= $(COPTS) $(COMPILE_FLAGS) $(INCLUDE_DIRS)

ifdef CHAPMS
CFLAGS+= -DCHAPMS=1
NEEDDES=y
PPPDOBJS+= md4.o chap_ms.o
HEADERS+= md4.h chap_ms.h
ifdef MSLANMAN
CFLAGS+= -DMSLANMAN=1
endif
ifdef MPPE
CFLAGS+= -DMPPE=1
endif
endif

# EAP SRP-SHA1
ifdef USE_SRP
CFLAGS	+= -DUSE_SRP -DOPENSSL -I/usr/local/ssl/include
LIBS	+= -lsrp -L/usr/local/ssl/lib -lcrypto
TARGETS	+= srp-entry
#EXTRAINSTALL = $(INSTALL) -s -c -m 555 srp-entry $(BINDIR)/srp-entry
MANPAGES += srp-entry.8
EXTRACLEAN += srp-entry.o
NEEDDES=y
else
# OpenSSL has an integrated version of SHA-1, and its implementation
# is incompatible with this local SHA-1 implementation.  We must use
# one or the other, not both.
PPPDSRCS += sha1.c
HEADERS += sha1.h
PPPDOBJS += sha1.o
endif

ifdef HAS_SHADOW
CFLAGS   += -DHAS_SHADOW
#LIBS     += -lshadow $(LIBS)
endif

ifneq ($(wildcard /usr/include/crypt.h),)
CFLAGS   += -DHAVE_CRYPT_H=1
endif
ifneq ($(wildcard /usr/lib/libcrypt.*),)
LIBS	+= -lcrypt
endif
# *** Leo *** It's unreasonable to check the host's libraries.
# So I add the linking library "crypt" that work around on UBICOM IP8K
ifeq ($(strip $(HOST_CPU)),ubicom32)
LIBS += -lcrypt
endif

ifdef NEEDDES
ifndef USE_CRYPT
LIBS     += -ldes $(LIBS)
else
CFLAGS   += -DUSE_CRYPT=1
endif
PPPDOBJS += pppcrypt.o
HEADERS += pppcrypt.h
endif

# For "Pluggable Authentication Modules", see ftp.redhat.com:/pub/pam/.
ifdef USE_PAM
CFLAGS   += -DUSE_PAM
LIBS     += -lpam -ldl
endif

# Multi-linnk
ifdef HAVE_MULTILINK
	# Multilink implies the use of TDB
	USE_TDB=y

	CFLAGS += -DHAVE_MULTILINK
	PPPDSRCS += multilink.c
	PPPDOBJS += multilink.o
endif

# TDB
ifdef USE_TDB
	CFLAGS += -DUSE_TDB=1
	PPPDSRCS += tdb.c
	PPPDOBJS += tdb.o
	HEADERS += tdb.h
endif

# Lock library binary for Linux is included in 'linux' subdirectory.
ifdef LOCKLIB
LIBS     += -llock
CFLAGS   += -DLOCKLIB=1
endif

ifdef PLUGIN
CFLAGS	+= -DPLUGIN
LDFLAGS	+= -Wl,-E
LIBS	+= -ldl
endif

ifdef FILTER
LIBS    += -lpcap
CFLAGS  += -DPPP_FILTER -I/usr/include/pcap
endif

#ifdef HAVE_INET6
ifeq ($(strip $(ELBOX_USE_IPV6)),y)
     PPPDSRCS += ipv6cp.c eui64.c
     HEADERS  += ipv6cp.h eui64.h
     PPPDOBJS += ipv6cp.o eui64.o
     CFLAGS   += -DINET6=1
endif

ifdef CBCP
     PPPDSRCS += cbcp.c
     PPPDOBJS += cbcp.o
     CFLAGS += -DCBCP_SUPPORT
     HEADERS += cbcp.h
endif

ifdef MAXOCTETS
     CFLAGS += -DMAXOCTETS
endif
ifdef UCLINUX
     CFLAGS += -DEMBED -DSTRIP
endif

.PHONY: all install clean accel-pptp accel-pptp-clean

all: accel-pptp $(TARGETS)

ifeq ($(strip $(ELBOX_PROGS_GPL_PPPD_ACCEL_PPTP)),y)
accel-pptp:
	@if [ -f ./accel-pptp/if_pppox.h ]; then \
		rm -f ./accel-pptp/if_pppox.h; \
	fi
	$(Q)ln -s $(KERNELDIR)/include/linux/if_pppox.h ./accel-pptp/if_pppox.h 
else
accel-pptp:
endif

ifeq ($(strip $(ELBOX_PROGS_GPL_PPPD_ACCEL_PPTP)),y)
accel-pptp-clean:
	$(Q)rm -f ./accel-pptp/if_pppox.h
else
accel-pptp-clean:
endif

pppd: $(PPPDOBJS)
	@echo -e "\033[32mBuilding pppd ...\033[0m"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o pppd $(PPPDOBJS) $(LIBS)
	$(Q)$(STRIP) pppd

srp-entry:	srp-entry.c
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ srp-entry.c $(LIBS)

clean: accel-pptp-clean
	@echo -e "\033[32mCleaning pppd ...\033[0m"
	$(Q)rm -f *.gdb *.elf $(PPPDOBJS) $(EXTRACLEAN) $(TARGETS) *~ #* core

install:
	@echo -e "\033[32mInstalling pppd ...\033[0m"
	$(Q)[ -d $(TARGET)/usr/sbin ] || mkdir -p $(TARGET)/usr/sbin
	$(Q)install pppd			$(TARGET)/usr/sbin
	$(Q)chmod +x $(TARGET)/usr/sbin/pppd
#ifndef RGAPPS_PROGS_PRIV_COMMON_SCRIPTS
#	$(Q)[ -d $(TARGET)/etc/scripts/ppp ] || mkdir -p $(TARGET)/etc/scripts/ppp
#	install ./scripts/pppif.sh	$(TARGET)/etc/scripts/ppp
#	install ./scripts/ppp-setup.sh	$(TARGET)/etc/scripts/ppp
#	install ./scripts/ppp.sh	$(TARGET)/etc/scripts/ppp
#	install ./scripts/ppp-loop.sh	$(TARGET)/etc/scripts/ppp
#	install ./scripts/ppp-status.sh	$(TARGET)/etc/scripts/ppp
#	chmod +x $(TARGET)/etc/scripts/ppp/pppif.sh
#	chmod +x $(TARGET)/etc/scripts/ppp/ppp-setup.sh
#	chmod +x $(TARGET)/etc/scripts/ppp/ppp.sh
#	chmod +x $(TARGET)/etc/scripts/ppp/ppp-loop.sh
#	chmod +x $(TARGET)/etc/scripts/ppp/ppp-status.sh
#endif

.c.o:
	$(Q)$(CC) -c $(CFLAGS) $< -o $@
