# makefile template

#include MakeInclude

LDLIBS =  

VLAN_OBJS = vconfig.o

ALL_OBJS = ${VLAN_OBJS}

VCONFIG = vconfig
CFLAGS += -D_GNU_SOURCE 
#LDFLAGS += -Wl,--static 

all: $(VCONFIG)

objects: $(VLAN_OBJS)

#This is pretty silly..
vconfig.h: Makefile
	$(Q)touch vconfig.h

$(VCONFIG): $(VLAN_OBJS)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $(VCONFIG) $(VLAN_OBJS) $(LDLIBS)
	$(Q)[ -z "$(STRIP)" ] || $(STRIP) $(VCONFIG)

macvlan_config: macvlan_config.c
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(ALL_OBJS): %.o: %.c %.h
	$(Q)$(CC) $(CFLAGS) -c $<

clean:
	$(Q)rm -f *.o $(VCONFIG) macvlan_config

purge: clean
	$(Q)rm -f *.flc ${VCONFIG} macvlan_config vconfig.h
	$(Q)rm -f *~

.PHONY: all objects clean
