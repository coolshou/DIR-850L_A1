###############################################################
# General rules
#
#	$(APPLET)  - target file
#	$(CPLUS_SRCS) - local %.cpp source files
#	$(LOCAL_SRCS) - local %.c source files
#	$(CMLIB_SRCS) - comlib %.c source files
#

##########################################################################
# check login user group
UNAME:=$(shell id -g)
ifeq ($(strip $(UNAME)),0)
%:
	@echo -e "\033[32m"; \
	echo "You are building code in superuser mode."; \
	echo -e "Please login in normal user mode \033[0m"; \
	echo ""
	exit 9
else

##########################################################################
-include $(TOPDIR)/.config

COMLIB:=$(TOPDIR)/comlib
COMINC:=$(TOPDIR)/include

CPLUS_OBJS:=$(patsubst %cpp,%o,$(CPLUS_SRCS))
LOCAL_OBJS:=$(patsubst %c,%o,$(LOCAL_SRCS))
CMLIB_OBJS:=$(patsubst %c,%o,$(CMLIB_SRCS))
COMLIBSRCS:=$(foreach T,$(CMLIB_SRCS),$(COMLIB)/$(T))

###############################################################
# Setup CFLAGS for comlib.
ifeq ($(strip $(ELBOX_COMLIB_MEM_HELPER_DISABLE)),y)
CFLAGS += -DCONFIG_MEM_HELPER_DISABLE=y
endif

###############################################################
# make dependency file
ifeq (.depend, $(wildcard .depend))
all: $(PREBUILD) $(APPLET)
	$(Q)for i in $(OTHER_TARGETS); do make $$i V=$(V); done

-include .depend
else
all: $(PREBUILD) .depend
ifneq ($(strip $(APPLET)),)
	$(Q)make $(APPLET)
endif
	$(Q)for i in $(OTHER_TARGETS); do make $$i V=$(V); done

.depend: Makefile $(LOCAL_SRCS) $(COMLIBSRCS)
	@echo -e "\033[32mCreating dependency file for APPLET: $(APPLET) ...\033[0m"
	$(Q)echo -n > .depend
ifneq ($(strip $(LOCAL_SRCS)),)
	$(Q)$(CC) -I$(COMINC) $(CFLAGS) -M $(LOCAL_SRCS) >> .depend
endif
ifneq ($(strip $(COMLIBSRCS)),)
	$(Q)$(CC) -I$(COMINC) $(CFLAGS) -M $(COMLIBSRCS) >> .depend
endif
ifneq ($(strip $(CPLUS_SRCS)),)
	$(Q)$(CC) -I$(COMINC) $(CFLAGS) -M $(CPLUS_SRCS) >> .depend
endif
endif

dep:
	$(Q)rm -f .depend
	$(Q)make .depend


ifneq ($(strip $(APPLET)),)
# build rule for APPLET.
$(APPLET): $(CPLUS_OBJS) $(LOCAL_OBJS) $(CMLIB_OBJS)
	@echo -e "\033[32mbuilding APPLET: $(APPLET) ...\033[0m"
ifeq ($(strip $(CPLUS_OBJS)),)
	$(Q)$(CC) $(LOCAL_OBJS) $(CMLIB_OBJS) $(LDFLAGS) -o $@
else
	$(Q)$(CXX) $(LOCAL_OBJS) $(CMLIB_OBJS) $(CPLUS_OBJS) $(LDFLAGS) -o $@
endif

# build rule for source in comlib.
$(CMLIB_OBJS): %.o: $(COMLIB)/%.c
	$(Q)$(CC) -I$(COMINC) $(CFLAGS) -c -o $@ $<

# build rule for our sources.
$(LOCAL_OBJS): %.o: %.c
	$(Q)$(CC) -I$(COMINC) $(CFLAGS) -c -o $@ $<

# build c++ sources
$(CPLUS_OBJS): %.o: %.cpp
	$(Q)$(CXX) -I$(COMINC) $(CFLAGS) -c -o $@ $<

clean_objs:
	@echo -e "\033[32mCleaning APPLET: $(APPLET) ...\033[0m"
	$(Q)rm -rf $(CPLUS_OBJS) $(LOCAL_OBJS) $(CMLIB_OBJS) $(APPLET) .depend

else
clean_objs:
	@echo -e "\033[32maCleaning APPLET: no applet !! ...\033[0m"
endif

.PHONY: generate_dep clean_objs

endif
##########################################################################
