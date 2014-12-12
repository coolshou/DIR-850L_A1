ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../../../../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/other/jpeg2000/libjasper/jpc
VPATH=.:$(SRCDIR)/$(SUBDIR)

JASPERSRCDIR=$(SRCDIR)/$(SUBDIR)/..

include $(BUILDDIR)/Makefile.config

LIB_OBJECTS = jpc_bs.o jpc_cs.o jpc_dec.o jpc_enc.o \
	jpc_math.o jpc_mct.o jpc_mqcod.o jpc_mqdec.o jpc_mqenc.o \
	jpc_qmfb.o jpc_tagtree.o jpc_t1cod.o jpc_t1dec.o jpc_t1enc.o \
	jpc_tsfb.o jpc_t2cod.o jpc_t2dec.o jpc_t2enc.o jpc_util.o

MERGE_OBJECTS =

all: partlist $(LIB_OBJECTS)

include $(JASPERSRCDIR)/Makefile.common

