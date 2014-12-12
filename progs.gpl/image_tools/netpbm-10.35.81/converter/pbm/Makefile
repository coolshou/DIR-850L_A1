ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/pbm
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

PORTBINARIES =	atktopbm brushtopbm cmuwmtopbm ddbugtopbm g3topbm escp2topbm \
		icontopbm macptopbm mdatopbm mgrtopbm mrftopbm \
		pbmto10x pbmto4425 pbmtoascii pbmtoatk \
		pbmtobbnbg pbmtocmuwm pbmtodjvurle \
		pbmtoepsi pbmtoepson pbmtoescp2 \
		pbmtog3 pbmtogem pbmtogo pbmtoibm23xx pbmtoicon pbmtolj \
		pbmtoln03 pbmtolps \
		pbmtomacp pbmtomatrixorbital pbmtomda pbmtomgr pbmtomrf \
		pbmtonokia \
		pbmtopi3 pbmtoplot pbmtopsg3 pbmtoptx pbmtowbmp \
		pbmtox10bm pbmtoxbm pbmtoybm pbmtozinc \
		pi3topbm pktopbm \
		wbmptopbm xbmtopbm ybmtopbm	

ifneq ($(LEX)x,x)
  PORTBINARIES += thinkjettopbm
endif

#pbmpage uses sqrt(), which is sometimes in libc, not libm.  Is it ever
#in libm?
MATHBINARIES =	pbmtopk
BINARIES =	$(PORTBINARIES) $(MATHBINARIES)
SCRIPTS =

OBJECTS = $(BINARIES:%=%.o)

MERGEBINARIES = $(BINARIES)
MERGE_OBJECTS = $(MERGEBINARIES:%=%.o2)

SUBDIRS=pbmtoppa

.PHONY: all
all: $(BINARIES) $(SUBDIRS:%=%/all)

include $(SRCDIR)/Makefile.common

ifneq ($(LEX)x,x)
thinkjettopbm.c1:%.c1:%.l
	$(LEX) -t $< >$@
endif

thinkjettopbm.c:%.c:%.c1 $(SRCDIR)/lib/util/lexheader
# Different versions of Lex produce subtly different output, from the
# same .l source file.  The .c1 file contains the raw output from Lex.
# We now massage it so it will compile.  We must add some definitions
# at the top (the lexheader file).  We must remove any yylex and
# yywrap prototype, as our .l file already contains one.  The Lex
# version "Software Generation Utilities (SGU) Solaris-ELF (4.0)"
# puts declarations for yylex and yywrap, as external symbols,
# into its output, causing a duplicate declaration error at compile time.
#
# Schwarb Manfred reports that it compiles OK, but with warnings, on
# Solaris.  Solaris Lex has a -e option that eliminates the lex
# warnings, but causes compiler warnings.  AIX and Flex don't have a
# -e option.  -Bryan 2001.05.16.
#
# But Peter Weisz reported on 2002.12.11 that on Solaris, compile
# failed due to a duplicate declaration of yylex and yywrap with Netpbm
# 10.12, which does not remove those declarations as the current version
# does.
	cat $(SRCDIR)/lib/util/lexheader $< | \
	  grep -v "^[[:space:]]*int yylex(void);" | \
	  grep -v "^[[:space:]]*int yywrap(void);" \
	  >$@

thisdirclean: localclean
.PHONY: localclean
localclean:
	-rm -f thinkjettopbm.c
