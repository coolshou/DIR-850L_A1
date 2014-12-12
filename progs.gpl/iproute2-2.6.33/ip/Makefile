-include ../../config.arch
-include ../../../config.path

CFLAGS += -I../include

PREFIX = /usr/sbin

IPOBJ=ip.o ipaddress.o ipaddrlabel.o iproute.o iprule.o \
    rtm_map.o iptunnel.o ip6tunnel.o tunnel.o ipneigh.o ipntable.o iplink.o \
    ipmaddr.o ipmonitor.o ipmroute.o ipprefix.o iptuntap.o \
    ipxfrm.o xfrm_state.o xfrm_policy.o xfrm_monitor.o \
    iplink_vlan.o link_veth.o link_gre.o iplink_can.o \
    iplink_macvlan.o

RTMONOBJ=rtmon.o

ALLOBJ=$(IPOBJ) $(RTMONOBJ)
SCRIPTS=ifcfg rtpr routel routef
#TARGETS=ip rtmon
TARGETS=ip

all: $(TARGETS) $(SCRIPTS) $(LIBS)
	$(STRIP) ./$(TARGETS)

ip: $(IPOBJ) $(LIBNETLINK) $(LIBUTIL)

rtmon: $(RTMONOBJ) $(LIBNETLINK)

install: all
	[ -d $(TARGEt)$(PREFIX) ] || mkdir -p $(TARGET)$(PREFIX)
	install ./$(TARGETS) $(TARGET)$(PREFIX)
	#install -m 0755 $(TARGETS) $(DESTDIR)$(SBINDIR)
	#install -m 0755 $(SCRIPTS) $(DESTDIR)$(SBINDIR)

clean:
	rm -f $(ALLOBJ) $(TARGETS) static-syms.o

SHARED_LIBS ?= y
ifeq ($(SHARED_LIBS),y)

LDLIBS += -ldl
LDFLAGS += -Wl,-export-dynamic

else

ip: static-syms.o
static-syms.o: static-syms.h
static-syms.h: $(wildcard *.c)
	files="$^" ; \
	for s in `grep -B 3 '\<dlsym' $$files | sed -n '/snprintf/{s:.*"\([^"]*\)".*:\1:;s:%s::;p}'` ; do \
		sed -n '/'$$s'[^ ]* =/{s:.* \([^ ]*'$$s'[^ ]*\) .*:extern char \1[] __attribute__((weak)); if (!strcmp(sym, "\1")) return \1;:;p}' $$files ; \
	done > $@

endif
