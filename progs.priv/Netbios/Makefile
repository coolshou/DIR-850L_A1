include path.mk

all:

clean:

install:
	$(Q)test ! -e $(TARGET)/prebuild && cp -Rf $(TOPDIR)/prebuild/* $(TARGET) || exit 0

.PHONY: all clean install
