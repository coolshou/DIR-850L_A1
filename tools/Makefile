TOOLS = $(shell find . -maxdepth 1 -type d ! -iname ".*" ! -iname "alpha" ! -iname "sqlzma")
all:

clean distclean:
	@for i in $(TOOLS); do make -C $$i $@; done
